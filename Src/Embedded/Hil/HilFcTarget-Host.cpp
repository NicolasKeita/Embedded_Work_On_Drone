/*
Filename: Src/Embedded/Hil/HilFcTarget-Host.cpp
Description: Host FC emulator target : drains a SensorPacket, validates and holds the
last good sample, runs the real Flight Controller core through the HAL sensor/actuator
abstractions and emits the answering ActuatorPacket.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilFcTarget;

import std;

import Aircraft;
import FlightController;
import FlightControllerTypes;
import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilSensorModel;
import HilTransport;
import SimActuatorOutput;
import SimSensorInput;
import Telemetry;
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

HostFcTarget::HostFcTarget(FlightCore::Transport::ITransport& channel,
                            IWallClock& clock,
                            const sim::control::TargetState& target,
                            const sim::control::ControllerConfig& controller,
                            std::float64_t dt,
                            const sim::sil::SensorValidationLimits& sensor_limits)
    : channel_{channel},
      clock_{clock},
      fc_{controller},
      target_{target},
      dt_{dt},
      sensor_limits_{sensor_limits}
{
}

FcStepOutcome HostFcTarget::respond(std::uint16_t expected_sequence)
{
    FcStepOutcome outcome{};

    FlightCore::Transport::HilHeader header{};
    if (!receive_frame(channel_, parser_, header, payload_buffer_)) {
        return outcome;
    }
    if (header.msg_id != FlightCore::Transport::kMsgIdSensor
        || header.payload_len < FlightCore::Transport::kSensorPayloadSize) {
        return outcome;
    }

    outcome.fc_receive_wall_us = clock_.nowUs();

    FlightCore::Transport::HilSensorPayload sensor_payload{};
    const std::span<const std::uint8_t> span(payload_buffer_.data(),
                                             FlightCore::Transport::kSensorPayloadSize);
    if (!FlightCore::Transport::decodeSensorPayload(span, sensor_payload)) {
        return outcome;
    }
    outcome.sequence = header.sequence_num;
    outcome.echo_sim_timestamp_us = sensor_payload.sim_timestamp_us;

    FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const sim::sil::SensorTelemetry telemetry = to_telemetry(sensor);
    if (sim::sil::validate(telemetry, sensor_limits_).all_valid()) {
        sensor_input_.inject(sensor);
    }
    FlightCore::HAL::SensorData consumed{};
    if (sensor_input_.readSensorData(consumed)) {
        fc_view_ = to_aircraft_state(consumed);
        have_view_ = true;
    }
    if (!have_view_) {
        fc_view_ = AircraftState{};
    }

    const ControlCommand command = fc_.update(target_, fc_view_, dt_);
    outcome.mission_state = static_cast<std::uint8_t>(fc_.state());
    FlightCore::HAL::ActuatorCommands cmds = to_actuator_commands(command);
    cmds.mode_flags = static_cast<std::uint8_t>(fc_.state());
    cmds.timestamp_us = clock_.nowUs();
    actuator_output_.writeActuatorCommands(cmds);
    const FlightCore::HAL::ActuatorCommands& emitted = actuator_output_.lastCommands();

    outcome.fc_send_wall_us = clock_.nowUs();
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{
        .cpu_usage_pct_x100 = 0,
        .stack_watermark_words = 0,
        .deadline_miss_count = 0,
        .fc_health_status = kFcHealthHealthy,
    };
    if (!send_actuator_frame(channel_, emitted, outcome.echo_sim_timestamp_us, diagnostics,
                             header.sequence_num)) {
        return outcome;
    }
    outcome.ok = true;
    return outcome;
}

}
