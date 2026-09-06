/*
Filename: Src/Embedded/Hil/Target/HilFcTarget-Respond.cpp
Description: Host FC emulator target response path : runs one Flight Controller cycle
for the received SensorPacket and emits the answering ActuatorPacket.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilFcTarget;

import std;

import Aircraft;
import FlightController;
import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilTransport;
import SimActuatorOutput;
import Transport;

namespace sim::hil {

namespace {
    constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;
    constexpr std::uint8_t   kFcHealthHealthy = 1;

    FlightCore::HAL::ActuatorCommands to_actuator_commands(const ControlCommand& cmd) noexcept
    {
        return FlightCore::HAL::ActuatorCommands{
            .timestamp_us = 0,
            .wing_rpm_cmd = static_cast<std::float32_t>(cmd.wing_rpm),
            .left_servo_rad = static_cast<std::float32_t>(cmd.left_servo_angle * kRadiansPerDegree),
            .right_servo_rad = static_cast<std::float32_t>(cmd.right_servo_angle * kRadiansPerDegree),
            .aux_actuator_cmd = 0.0f,
            .mode_flags = static_cast<std::uint8_t>(0),
        };
    }
}

bool HostFcTarget::send_actuators(const ControlCommand& command, FcStepOutcome& outcome)
{
    FlightCore::HAL::ActuatorCommands cmds = to_actuator_commands(command);
    cmds.mode_flags = static_cast<std::uint8_t>(fc_.state());
    cmds.timestamp_us = clock_.nowUs();
    if (!actuator_output_.writeActuatorCommands(cmds)) {
        return false;
    }
    const FlightCore::HAL::ActuatorCommands& emitted = actuator_output_.lastCommands();

    outcome.fc_send_wall_us = clock_.nowUs();
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{
        .cpu_usage_pct_x100 = 0,
        .stack_watermark_words = 0,
        .deadline_miss_count = 0,
        .fc_health_status = kFcHealthHealthy,
    };
    if (!send_actuator_frame(channel_, emitted, outcome.echo_sim_timestamp_us, diagnostics, outcome.sequence)) {
        return false;
    }
    outcome.ok = true;
    return true;
}

FcStepOutcome HostFcTarget::respond(std::uint16_t expected_sequence)
{
    FcStepOutcome outcome{};
    FlightCore::Transport::HilSensorPayload sensor_payload{};

    if (!receive_sensor(expected_sequence, outcome, sensor_payload)) {
        return outcome;
    }
    const ControlCommand command = update_control(sensor_payload);
    outcome.mission_state = static_cast<std::uint8_t>(fc_.state());
    send_actuators(command, outcome);
    return outcome;
}

}