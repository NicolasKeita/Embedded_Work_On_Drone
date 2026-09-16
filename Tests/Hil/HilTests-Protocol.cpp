/*
Filename: Tests/Hil/HilTests-Protocol.cpp
Description: HIL inter-FC protocol test : verifies that the inter-FC status message
and its CRC survive a complete frame round trip through the byte-by-byte parser.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import Aircraft;
import FlightControllerTypes;
import HalTypes;
import HilClock;
import HilFcTarget;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilTransport;
import InterFcLink;
import LoopbackTransport;
import TestHarness;

namespace sim::test::hil {

namespace {
    /* Changes the wire setpoint during one control session without resetting the FC. */
    void test_dynamic_setpoint(sim::test::TestHarness& runner)
    {
        FlightCore::Sim::LoopbackTransport channel{};
        sim::hil::HilTransport transport{&channel};
        sim::hil::MonotonicClock clock{};
        sim::hil::HostFcTargetConfig config{};
        config.controller.hover_rpm = Aircraft{}.hover_rpm();
        config.controller.spin_up_seconds = 0.01;
        config.controller.takeoff_transition_seconds = 0.01;
        config.dt = 0.01;
        sim::hil::HostFcTarget target{channel, clock, config};
        FlightCore::HAL::SensorData sensor{};
        sensor.position_z_m = 10.0f;
        sensor.altitude_baro_m = 10.0f;
        sensor.wing_rpm_meas = static_cast<std::float32_t>(config.controller.hover_rpm);
        sensor.sensor_valid_flags = 0x1Fu;
        FlightCore::Transport::HilControlSetpoint setpoint{
            .target_z_m = 10.0f,
            .station_hold_seconds = 120.0f,
        };
        std::float32_t hover_command = 0.0f;
        for (std::uint16_t step = 0; step < 7; ++step) {
            sensor.timestamp_us = static_cast<std::uint64_t>(step) * 10000u;
            setpoint.target_z_m = step >= 5 ? 30.0f : 10.0f;
            const std::uint64_t sent = clock.nowUs();
            runner.check(transport.sendSensor(sensor, step, setpoint), "dynamic setpoint sent");
            const sim::hil::FcStepOutcome response = target.respond(step);
            runner.check(response.ok, "same FC instance accepts the updated setpoint");
            FlightCore::HAL::ActuatorCommands commands{};
            FlightCore::Transport::ActuatorDiagnostics diagnostics{};
            std::int64_t rtt = 0;
            const auto received = transport.receiveActuator(clock, clock.nowUs() + 50000,
                sim::hil::ActuatorExpectations{step, sensor.timestamp_us, sent},
                sim::hil::ActuatorReceiveOutputs{commands, diagnostics, rtt});
            runner.check(received == sim::hil::ReceiveResult::Ok, "dynamic control response decoded");
            if (step == 4) {
                hover_command = commands.wing_rpm_cmd;
            }
            if (step >= 5) {
                runner.check(commands.wing_rpm_cmd > hover_command + 1.0f,
                             "changing 10 m to 30 m increases thrust without restarting control");
                runner.check(commands.mode_flags == static_cast<std::uint8_t>(sim::control::MissionState::STATION_KEEPING),
                             "setpoint update preserves the current control phase");
            }
        }
    }

    /* Exercises the control setpoint through the real sensor transport and parser. */
    void test_control_setpoint(sim::test::TestHarness& runner)
    {
        FlightCore::Sim::LoopbackTransport channel{};
        sim::hil::HilTransport transport{&channel};
        const FlightCore::HAL::SensorData sensor{.timestamp_us = 1234};
        const FlightCore::Transport::HilControlSetpoint setpoint{
            .target_x_m = 4.0f,
            .target_y_m = -3.0f,
            .target_z_m = 30.0f,
            .station_hold_seconds = 120.0f,
        };
        runner.check(transport.sendSensor(sensor, 42, setpoint), "control setpoint sent");
        FlightCore::Transport::HilFrameParser parser{};
        FlightCore::Transport::HilHeader header{};
        std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> bytes{};
        runner.check(sim::hil::receive_frame(channel, parser, header, bytes), "setpoint frame parsed");
        FlightCore::Transport::HilSensorPayload decoded{};
        runner.check(FlightCore::Transport::decodeSensorPayload(
                         std::span<const std::uint8_t>{bytes.data(), header.payload_len}, decoded),
                     "setpoint payload decoded");
        runner.check(header.protocol_ver == 0x11 && header.sequence_num == 42
                         && header.payload_len == 96 && decoded.sim_timestamp_us == 1234,
                     "v1.1 frame layout and sensor timestamp preserved");
        runner.check(decoded.setpoint.target_x_m == 4.0f && decoded.setpoint.target_y_m == -3.0f
                         && decoded.setpoint.target_z_m == 30.0f
                         && decoded.setpoint.station_hold_seconds == 120.0f,
                     "3D target and hold duration survive the wire round trip");
        runner.check(!FlightCore::Transport::decodeSensorPayload(
                         std::span<const std::uint8_t>{bytes.data(), 80}, decoded),
                     "legacy sensor payload without control setpoint rejected");
        const auto defaults = FlightCore::Transport::makeSensorPayload(sensor, {});
        runner.check(defaults.setpoint.target_z_m == 0.0f && defaults.setpoint.station_hold_seconds == 5.0f,
                     "initial target is zero until supplied by the runner");
    }

    /* Rejects malformed setpoint values before either FC implementation uses them. */
    void test_invalid_control_setpoint(sim::test::TestHarness& runner)
    {
        const std::float32_t nan = std::numeric_limits<std::float32_t>::quiet_NaN();
        const std::float32_t infinity = std::numeric_limits<std::float32_t>::infinity();
        const std::array<FlightCore::Transport::HilControlSetpoint, 6> invalid{{
            {.target_x_m = nan},
            {.target_y_m = infinity},
            {.target_z_m = nan},
            {.target_z_m = -1.0f},
            {.station_hold_seconds = infinity},
            {.station_hold_seconds = -1.0f},
        }};
        for (const auto& setpoint : invalid) {
            const auto payload = FlightCore::Transport::makeSensorPayload({}, setpoint);
            const auto bytes = std::bit_cast<std::array<std::uint8_t, FlightCore::Transport::kSensorPayloadSize>>(payload);
            FlightCore::Transport::HilSensorPayload decoded{};
            runner.check(!FlightCore::Transport::decodeSensorPayload(bytes, decoded),
                         "invalid control setpoint rejected");
        }
    }

    /* Rejects a legacy version even with a valid CRC, then accepts the next current frame. */
    void test_protocol_version(sim::test::TestHarness& runner)
    {
        const auto payload = FlightCore::Transport::makeSensorPayload({}, {});
        auto frame = FlightCore::Transport::encodeSensorFrame(payload, 0);
        frame[3] = 0x10;
        const std::uint16_t crc = FlightCore::Transport::HilCrc::compute(
            std::span<const std::uint8_t>{frame.data(), frame.size() - 2});
        frame[frame.size() - 2] = static_cast<std::uint8_t>(crc & 0xFFu);
        frame[frame.size() - 1] = static_cast<std::uint8_t>(crc >> 8);
        FlightCore::Transport::HilFrameParser parser{};
        FlightCore::Transport::HilHeader header{};
        std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> bytes{};
        bool accepted = false;
        for (const std::uint8_t byte : frame) {
            accepted = parser.processByte(byte, header, bytes) || accepted;
        }
        runner.check(!accepted, "legacy protocol version rejected despite valid CRC");
        frame = FlightCore::Transport::encodeSensorFrame(payload, 1);
        for (const std::uint8_t byte : frame) {
            accepted = parser.processByte(byte, header, bytes) || accepted;
        }
        runner.check(accepted && header.sequence_num == 1, "parser recovers on the next v1.1 frame");
    }

    /* Verifies that the inter-FC status and its CRC survive a complete frame round trip. */
    void test_inter_fc_status(sim::test::TestHarness& runner)
    {
        const FlightCore::InterFc::Message           status{
            .kind = FlightCore::InterFc::MessageKind::Status,
            .sequence = 42,
            .state = FlightCore::InterFc::NodeState::Safe,
            .detection = FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout,
        };
        const FlightCore::InterFc::FrameCodec::Frame frame = FlightCore::InterFc::FrameCodec::encode(status);
        FlightCore::InterFc::FrameParser             parser{};
        std::optional<FlightCore::InterFc::Message>  decoded{};

        for (const std::uint8_t byte : frame) {
            const std::optional<FlightCore::InterFc::Message> candidate = parser.process(byte);
            if (candidate.has_value()) {
                decoded = candidate;
            }
        }
        runner.check(decoded.has_value(), "inter-FC status frame accepted");
        runner.check(decoded.has_value() && decoded->kind == FlightCore::InterFc::MessageKind::Status,
                     "inter-FC status kind preserved");
        runner.check(decoded.has_value() && decoded->sequence == 42, "inter-FC status sequence preserved");
        runner.check(decoded.has_value() && decoded->state == FlightCore::InterFc::NodeState::Safe,
                     "inter-FC SAFE state preserved");
        runner.check(decoded.has_value()
                         && decoded->detection == FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout,
                     "inter-FC detection code preserved");
    }
}

void run_protocol_tests(sim::test::TestHarness& runner)
{
    runner.set_context("PROTO");
    run_transport_tests(runner);
    test_inter_fc_status(runner);
    test_control_setpoint(runner);
    test_dynamic_setpoint(runner);
    test_invalid_control_setpoint(runner);
    test_protocol_version(runner);
}

}
