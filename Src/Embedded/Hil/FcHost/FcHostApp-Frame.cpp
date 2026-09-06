/*
Filename: Src/Embedded/Hil/FcHost/FcHostApp-Frame.cpp
Description: Frame handling of the host FC emulator : SensorPacket validation, FC cycle
with last-good-sample hold and ActuatorPacket emission.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <cstdio>

module FcHostApp;

import std;

import Aircraft;
import FlightController;
import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilSensorModel;
import Telemetry;

namespace {

constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;

FlightCore::HAL::ActuatorCommands to_actuator(const ControlCommand& cmd, std::uint64_t stamp_us) noexcept
{
    return FlightCore::HAL::ActuatorCommands{
        .timestamp_us = stamp_us,
        .wing_rpm_cmd = static_cast<std::float32_t>(cmd.wing_rpm),
        .left_servo_rad = static_cast<std::float32_t>(cmd.left_servo_angle * kRadiansPerDegree),
        .right_servo_rad = static_cast<std::float32_t>(cmd.right_servo_angle * kRadiansPerDegree),
        .aux_actuator_cmd = 0.0f,
        .mode_flags = 0,
    };
}

void answer_sensor(sim::hil::FcHostState&                         state,
                   const FlightCore::Transport::HilSensorPayload& sensor_payload,
                   std::uint16_t                                  sequence)
{
    const FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const sim::sil::SensorTelemetry   telemetry = sim::hil::to_telemetry(sensor);

    if (sim::sil::validate(telemetry, state.limits).all_valid()) {
        state.fc_view = sim::hil::to_aircraft_state(sensor);
        state.have_view = true;
    }
    if (!state.have_view) {
        state.fc_view = AircraftState{};
    }
    const ControlCommand command = state.fc.update(state.target, state.fc_view, state.dt);
    FlightCore::HAL::ActuatorCommands cmds = to_actuator(command, state.clock.nowUs());
    cmds.mode_flags = static_cast<std::uint8_t>(state.fc.state());
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{};
    const auto actuator_payload =
        FlightCore::Transport::makeActuatorPayload(cmds, sensor_payload.sim_timestamp_us, diagnostics);
    const auto actuator_frame = FlightCore::Transport::encodeActuatorFrame(actuator_payload, sequence);
    std::fwrite(actuator_frame.data(), 1, actuator_frame.size(), stdout);
    std::fflush(stdout);
}

}

namespace sim::hil {

bool process_frame(FcHostState& state, const FlightCore::Transport::HilHeader& header)
{
    if (header.msg_id != FlightCore::Transport::kMsgIdSensor
        || header.payload_len < FlightCore::Transport::kSensorPayloadSize) {
        return false;
    }
    FlightCore::Transport::HilSensorPayload sensor_payload{};
    const std::span<const std::uint8_t> span(state.payload_buffer.data(), FlightCore::Transport::kSensorPayloadSize);
    if (!FlightCore::Transport::decodeSensorPayload(span, sensor_payload)) {
        return false;
    }
    answer_sensor(state, sensor_payload, header.sequence_num);
    return true;
}

}
