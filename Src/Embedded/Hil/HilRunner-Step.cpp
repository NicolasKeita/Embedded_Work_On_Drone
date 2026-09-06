/*
Filename: Src/Embedded/Hil/HilRunner-Step.cpp
Description: One closed-loop HIL step : builds the sensor packet from the aircraft ground
truth, exchanges it with the FC target over the transport, evaluates host-side safety,
and applies the returned actuator command to the aircraft (with efficiency loss and the
SAFE_MODE/COMPENSATED overrides mirroring the SIL actuator step).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import Aircraft;
import CommsBus;
import FlightController;
import HalTypes;
import HealthMonitor;
import HilEvents;
import HilFcTarget;
import HilProtocol;
import HilConfig;
import HilClock;
import HilRunnerContext;
import HilSensorModel;
import HilTransport;
import SafetyManager;
import SilTypes;
import Telemetry;

namespace sim::hil {

namespace {
    constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;

    ControlCommand to_control_command(const FlightCore::HAL::ActuatorCommands& cmds) noexcept
    {
        return ControlCommand{
            .wing_rpm = cmds.wing_rpm_cmd,
            .left_servo_angle = cmds.left_servo_rad * kDegreesPerRadian,
            .right_servo_angle = cmds.right_servo_rad * kDegreesPerRadian,
        };
    }

    std::string_view receive_result_detail(ReceiveResult result) noexcept
    {
        switch (result) {
        case ReceiveResult::SequenceError:
            return "sequence number mismatch";
        case ReceiveResult::MessageTypeError:
            return "unexpected message id";
        case ReceiveResult::EchoMismatch:
            return "stale sim-timestamp echo";
        case ReceiveResult::InvalidPayload:
            return "invalid payload";
        case ReceiveResult::Timeout:
            return "actuator timeout";
        case ReceiveResult::SendFailed:
            return "sensor send failed";
        case ReceiveResult::Ok:
            return "ok";
        }
        return "unknown";
    }

    void record_step_error(HilRunContext& ctx, ReceiveResult result)
    {
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "HIL_RUNNER",
                                  .type = HilEventType::HilStepError,
                                  .severity = HilEventSeverity::Warning,
                                  .detail = receive_result_detail(result),
                                  .reason = "actuator packet rejected"});
    }
}

/*
Simulator-to-FC half of the closed loop: builds the sensor measurement from the private
ground truth (never the ground truth itself), sends the SensorPacket, lets the FC target
respond, models the FC1-FC2/actuator-link delivery through the reused CommsBus and
captures the ActuatorPacket (or drains a logically dropped one so the channel stays
clean for the next lockstep).
*/
void exchange_actuators(HilRunContext& ctx)
{
    ctx.sampled_truth = ctx.aircraft.state();
    ctx.sensors = ctx.sensor_model.sample(ctx.sampled_truth);
    if (ctx.env.sensor_corruption != sim::sil::SensorCorruptionMode::None) {
        ctx.sensors = sim::sil::apply_corruption(ctx.sensors, ctx.env.sensor_corruption, ctx.env.corrupted_altitude_m);
    }
    const sim::sil::SensorValidity validity = sim::sil::validate(ctx.sensors, ctx.config.sensor_limits);
    const FlightCore::HAL::SensorData wire = to_sensor_data(ctx.sensors, ctx.sim_ts_us, ctx.sampled_truth, validity);
    ctx.last_sensor_data = wire;

    const std::uint16_t sequence = static_cast<std::uint16_t>(ctx.step);
    const std::uint64_t sensor_send_wall = ctx.clock->nowUs();
    ctx.sensor_send_wall_us = sensor_send_wall;
    ctx.actuator_receive_wall_us = 0;
    if (!ctx.transport.sendSensor(wire, sequence)) {
        record_step_error(ctx, ReceiveResult::SendFailed);
        return;
    }

    ctx.this_fc = FcStepOutcome{};
    ctx.this_received = false;
    ctx.this_rtt_us = -1;

    if (!ctx.env.fc1_alive) {
        ctx.transport.noteTimeoutFrame();
        return;
    }
    ctx.this_fc = ctx.fc_target->respond(sequence);
    const sim::sil::CommsDelivery delivery = ctx.comms.publish(ctx.time);

    FlightCore::Transport::ActuatorDiagnostics diag{};
    if (delivery.delivered) {
        const ReceiveResult result = ctx.transport.receiveActuator(*ctx.clock, sequence, ctx.sim_ts_us,
                                                                     sensor_send_wall, ctx.next_deadline_us,
                                                                     ctx.actuator_cmd, diag, ctx.this_rtt_us);
        ctx.this_received = (result == ReceiveResult::Ok);
        if (ctx.this_received) {
            ctx.actuator_receive_wall_us =
                sensor_send_wall + static_cast<std::uint64_t>(std::max<std::int64_t>(0, ctx.this_rtt_us));
        }
        else {
            record_step_error(ctx, result);
        }
        return;
    }
    ctx.channel.flush();
    ctx.transport.noteDroppedFrame();
}

/*
Host-side health/safety evaluation (the FC2 monitoring role), reusing the SIL
HealthMonitor/SafetyManager so the detection chain matches the validated baseline.
*/
void update_health_and_safety(HilRunContext& ctx)
{
    const sim::safety::HealthReport report =
        ctx.health.evaluate(ctx.time, ctx.comms, ctx.sensors, ctx.commanded_rpm);
    ctx.safety_command = ctx.safety.update(ctx.time, report);
    record_detection(ctx, report);
    record_safety_transitions(ctx);

    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
    ctx.result.degraded_reached = ctx.result.degraded_reached || report.state == sim::safety::HealthState::DEGRADED;
    ctx.result.compensated_reached =
        ctx.result.compensated_reached || ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE;
}

/*
Applies the returned actuator command to the aircraft: holds the last command on a
missed response, engages the SAFE_MODE controlled descent or the COMPENSATED thrust
margin, then scales by the actuator efficiency and integrates the physics. The aircraft
state evolves only from actuator commands — it is never replayed.
*/
void apply_actuators(HilRunContext& ctx)
{
    const HilConfig& cfg = ctx.config;
    ControlCommand effective = ctx.this_received ? to_control_command(ctx.actuator_cmd) : ctx.command;

    if (ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE) {
        ctx.safe_rpm = std::max(std::float64_t{0.0}, ctx.last_effective_rpm - cfg.safe_descent_rpm_rate * cfg.dt_s);
        effective.wing_rpm = ctx.safe_rpm;
        effective.left_servo_angle = 0.0;
        effective.right_servo_angle = 0.0;
    }
    else if (ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED) {
        effective.wing_rpm *= ctx.safety_command.thrust_margin;
    }
    ctx.command = effective;
    ctx.commanded_rpm = effective.wing_rpm;
    ctx.last_effective_rpm = effective.wing_rpm;
    effective.wing_rpm *= ctx.env.actuator_efficiency;
    ctx.aircraft.set_command(effective);
    ctx.aircraft.update(cfg.dt_s);
}

}
