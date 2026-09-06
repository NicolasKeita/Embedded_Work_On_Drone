/*
Filename: Src/Embedded/Hil/FcHostMain.cpp
Description: Entry point of fc1_hil_host : the host FC emulator target. Reads HIL-Proto
SensorPacket frames from standard input, runs the REAL sim::control::FlightController
core (the same core the future fc1_stm32 firmware will run — not a HIL-specific
controller), validates and holds the last good sample on an invalid sensor, and writes
the answering ActuatorPacket frames to standard output. This is NOT the physical target;
it exists only to drive the HIL runner before the STM32 arrives and shares the FC
algorithm with the future embedded build.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

#if defined(_WIN32)
#include <io.h>
#include <fcntl.h>
#endif
#include <cstdio>

import std;

import Aircraft;
import FlightController;
import FlightControllerTypes;
import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilScenarios;
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

}

int main()
{
#if defined(_WIN32)
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    const sim::hil::HilConfig base = sim::hil::hil_base_config();
    sim::control::FlightController fc{base.controller};
    const sim::control::TargetState target = base.target;
    const std::float64_t dt = base.dt_s;
    const sim::sil::SensorValidationLimits limits = base.sensor_limits;
    sim::hil::MonotonicClock clock;

    AircraftState fc_view{};
    bool have_view = false;
    FlightCore::Transport::HilFrameParser parser{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload_buffer{};

    std::array<std::uint8_t, FlightCore::Transport::kSensorFrameSize> frame{};
    for (;;) {
        const std::size_t got = std::fread(frame.data(), 1, frame.size(), stdin);
        if (got != frame.size()) {
            break;
        }
        FlightCore::Transport::HilHeader header{};
        for (std::uint8_t byte : frame) {
            if (parser.processByte(byte, header, payload_buffer)) {
                if (header.msg_id != FlightCore::Transport::kMsgIdSensor
                    || header.payload_len < FlightCore::Transport::kSensorPayloadSize) {
                    break;
                }
                FlightCore::Transport::HilSensorPayload sensor_payload{};
                const std::span<const std::uint8_t> span(payload_buffer.data(),
                                                         FlightCore::Transport::kSensorPayloadSize);
                if (!FlightCore::Transport::decodeSensorPayload(span, sensor_payload)) {
                    break;
                }
                const FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
                const sim::sil::SensorTelemetry telemetry = sim::hil::to_telemetry(sensor);
                if (sim::sil::validate(telemetry, limits).all_valid()) {
                    fc_view = sim::hil::to_aircraft_state(sensor);
                    have_view = true;
                }
                if (!have_view) {
                    fc_view = AircraftState{};
                }
                const ControlCommand command = fc.update(target, fc_view, dt);
                FlightCore::HAL::ActuatorCommands cmds = to_actuator(command, clock.nowUs());
                cmds.mode_flags = static_cast<std::uint8_t>(fc.state());
                const FlightCore::Transport::ActuatorDiagnostics diagnostics{};
                const auto actuator_payload =
                    FlightCore::Transport::makeActuatorPayload(cmds, sensor_payload.sim_timestamp_us, diagnostics);
                const auto actuator_frame = FlightCore::Transport::encodeActuatorFrame(actuator_payload, header.sequence_num);
                std::fwrite(actuator_frame.data(), 1, actuator_frame.size(), stdout);
                std::fflush(stdout);
            }
        }
    }
    return 0;
}
