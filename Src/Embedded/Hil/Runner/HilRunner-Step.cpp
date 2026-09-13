/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Step.cpp
Description: Simulator-to-FC half of one closed-loop HIL step, including embedded
inter-FC fault commands and physical-FC2 diagnostic capture.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import Aircraft;
import CommsBus;
import HalTypes;
import HilClock;
import HilConfig;
import HilFcTarget;
import HilProtocol;
import HilRunnerContext;
import HilRunnerEvents;
import HilRunnerSafety;
import HilSensorModel;
import HilTransport;
import SilTypes;
import Telemetry;

namespace sim::hil {

namespace {
    /*
    FC-to-simulator half of the step: lets the FC target respond, models the
    FC1-FC2/actuator-link delivery through the reused CommsBus and captures the
    ActuatorPacket (or drains a logically dropped one so the channel stays clean for
    the next lockstep).
    */
    void receive_actuator_answer(HilRunContext& ctx, std::uint16_t sequence, std::uint64_t sensor_send_wall)
    {
        ctx.this_fc = ctx.fc_target->respond(sequence);
        bool delivered = true;
        if (!uses_embedded_fc2_supervision(ctx)) {
            delivered = ctx.comms.publish(ctx.time).delivered;
        }

        FlightCore::Transport::ActuatorDiagnostics diag{};
        if (!delivered) {
            ctx.channel->flush();
            ctx.transport.noteDroppedFrame();
            return;
        }
        const ActuatorExpectations expectations{.expected_sequence = sequence,
                                                 .expected_echo_sim_us = ctx.sim_ts_us,
                                                 .sensor_send_wall_us = sensor_send_wall};
        const ActuatorReceiveOutputs outputs{.commands = ctx.actuator_cmd,
                                              .diagnostics = diag,
                                              .round_trip_us = ctx.this_rtt_us};
        const ReceiveResult result = ctx.transport.receiveActuator(ctx.clock, ctx.next_deadline_us,
                                                                   expectations, outputs);
        ctx.this_received = (result == ReceiveResult::Ok);
        if (ctx.this_received) {
            ctx.actuator_diagnostics = diag;
            ctx.this_fc.sequence = sequence;
            ctx.this_fc.echo_sim_timestamp_us = ctx.sim_ts_us;
            ctx.this_fc.mission_state = ctx.actuator_cmd.mode_flags;
            ctx.actuator_receive_wall_us =
                sensor_send_wall + static_cast<std::uint64_t>(std::max<std::int64_t>(0, ctx.this_rtt_us));
        }
        else {
            record_step_error(ctx, result);
        }
    }
}

/*
Builds the sensor measurement from the private ground truth, applies any embedded
inter-FC test command, sends the SensorPacket and drives the FC answer half.
*/
void exchange_actuators(HilRunContext& ctx)
{
    ctx.sampled_truth = ctx.aircraft.state();
    ctx.sensors = ctx.sensor_model.sample(ctx.sampled_truth);
    if (ctx.env.sensor_corruption != sim::sil::SensorCorruptionMode::None) {
        ctx.sensors = sim::sil::apply_corruption(ctx.sensors, ctx.env.sensor_corruption, ctx.env.corrupted_altitude_m);
    }
    const sim::sil::SensorValidity validity = sim::sil::validate(ctx.sensors, ctx.config.sensor_limits);
    FlightCore::HAL::SensorData wire = to_sensor_data(ctx.sensors, ctx.sim_ts_us, ctx.sampled_truth, validity);
    const bool suppress_inter_fc_heartbeat = uses_embedded_fc2_supervision(ctx)
        && (!ctx.env.fc1_alive || !ctx.env.comms_link_up);
    if (suppress_inter_fc_heartbeat) {
        wire.sensor_valid_flags |= FlightCore::Transport::kHilCommandSuppressInterFcHeartbeat;
    }
    ctx.last_sensor_data = wire;

    const std::uint16_t sequence = static_cast<std::uint16_t>(ctx.step);
    const std::uint64_t sensor_send_wall = ctx.clock.nowUs();
    ctx.sensor_send_wall_us = sensor_send_wall;
    ctx.actuator_receive_wall_us = 0;
    if (!ctx.transport.sendSensor(wire, sequence)) {
        record_step_error(ctx, ReceiveResult::SendFailed);
        return;
    }

    ctx.this_fc = FcStepOutcome{};
    ctx.this_received = false;
    ctx.this_rtt_us = -1;

    if (!ctx.env.fc1_alive && !uses_embedded_fc2_supervision(ctx)) {
        ctx.transport.noteTimeoutFrame();
        return;
    }
    receive_actuator_answer(ctx, sequence, sensor_send_wall);
}

}
