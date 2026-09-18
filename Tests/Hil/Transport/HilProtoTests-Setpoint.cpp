/*
Filename: Tests/Hil/Transport/HilProtoTests-Setpoint.cpp
Description: HIL control setpoint tests: exercises the setpoint through the real sensor
transport and parser, and rejects malformed setpoint values before either FC uses them.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilProtoTests;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import HilTransport;
import LoopbackTransport;
import TestHarness;

namespace sim::test::hil {

namespace {
    /* Exercises the control setpoint through the real sensor transport and parser. */
    void test_control_setpoint(sim::test::TestHarness& runner)
    {
        FlightCore::Sim::LoopbackTransport              channel{};
        sim::hil::HilTransport                          transport{&channel};
        const FlightCore::HAL::SensorData               sensor{.timestamp_us = 1234};
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
        using Setpoint = FlightCore::Transport::HilControlSetpoint;

        const std::float32_t nan = std::numeric_limits<std::float32_t>::quiet_NaN();
        const std::float32_t infinity = std::numeric_limits<std::float32_t>::infinity();
        const std::array<Setpoint, 6> invalid{{
            {.target_x_m = nan},
            {.target_y_m = infinity},
            {.target_z_m = nan},
            {.target_z_m = -1.0f},
            {.station_hold_seconds = infinity},
            {.station_hold_seconds = -1.0f},
        }};
        for (const auto& setpoint : invalid) {
            const auto payload = FlightCore::Transport::makeSensorPayload({}, setpoint);
            const auto bytes =
                std::bit_cast<std::array<std::uint8_t, FlightCore::Transport::kSensorPayloadSize>>(payload);
            FlightCore::Transport::HilSensorPayload decoded{};
            runner.check(!FlightCore::Transport::decodeSensorPayload(bytes, decoded),
                         "invalid control setpoint rejected");
        }
    }
}

void run_proto_setpoint_tests(sim::test::TestHarness& runner)
{
    test_control_setpoint(runner);
    test_invalid_control_setpoint(runner);
}

}
