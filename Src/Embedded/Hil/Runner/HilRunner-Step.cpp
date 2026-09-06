/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Step.cpp
Description: Simulator-to-FC half of one closed-loop HIL step : builds the sensor packet
from the aircraft ground truth, sends it to the FC target over the transport and captures
the answering actuator packet (or the drop/timeout outcome).

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
        const sim::sil::CommsDelivery delivery = ctx.comms.publish(ctx.time);

        FlightCore::Transport::ActuatorDiagnostics diag{};
        if (!delivery.delivered) {
            ctx.channel.flush();
            ctx.transport.noteDroppedFrame();
            return;
        }
        const ReceiveResult result = ctx.transport.receiveActuator(*ctx.clock, sequence, ctx.sim_ts_us,
                                                                   sensor_send_wall, ctx.next_deadline_us,
                                                                   ctx.actuator_cmd, diag,
                                                                   ctx.this_rtt_us);
        ctx.this_received = (result == ReceiveResult::Ok);
        if (ctx.this_received) {
            ctx.actuator_receive_wall_us =
                sensor_send_wall + static_cast<std::uint64_t>(std::max<std::int64_t>(0, ctx.this_rtt_us));
        }
        else {
            record_step_error(ctx, result);
        }
    }
}

/*
Builds the sensor measurement from the private ground truth (never the ground truth
itself), sends the SensorPacket and drives the FC answer half of the lockstep.
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
    receive_actuator_answer(ctx, sequence, sensor_send_wall);
}

}
