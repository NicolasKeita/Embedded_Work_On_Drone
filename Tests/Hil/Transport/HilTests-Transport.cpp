/*
Filename: Tests/Hil/Transport/HilTests-Transport.cpp
Description: HIL transport-layer protocol tests : sequence, stale-echo, message-type
and CRC validation of the Sensor/Actuator frame path.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import HalTypes;
import HilClock;
import HilProtocol;
import HilProtocolCodec;
import HilTransport;
import LoopbackTransport;
import TestHarness;

namespace sim::test::hil {

namespace {
    bool push_actuator(FlightCore::Sim::LoopbackTransport& channel, std::uint16_t seq, std::uint64_t echo_us)
    {
        const auto                          cmds = FlightCore::HAL::ActuatorCommands{
            .timestamp_us = 7, .wing_rpm_cmd = 1000.0f + static_cast<std::float32_t>(seq),
            .left_servo_rad = 0.05f, .right_servo_rad = -0.05f, .aux_actuator_cmd = 0.0f, .mode_flags = 2,
        };
        const sim::hil::ActuatorDiagnostics diag{};
        const auto                          payload = FlightCore::Transport::makeActuatorPayload(cmds, echo_us, diag);
        const auto                          frame = FlightCore::Transport::encodeActuatorFrame(payload, seq);

        return channel.sendBytes(frame);
    }

    struct Fixture {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport             transport{&channel};
        sim::hil::MonotonicClock           clock;

        sim::hil::ReceiveResult receive(std::uint16_t expected_seq, std::uint64_t expected_echo)
        {
            FlightCore::HAL::ActuatorCommands out{};
            sim::hil::ActuatorDiagnostics     diag{};
            std::int64_t                      rtt = 0;

            return transport.receiveActuator(clock, clock.nowUs() + 50000,
                                            sim::hil::ActuatorExpectations{expected_seq, expected_echo, 0},
                                            sim::hil::ActuatorReceiveOutputs{out, diag, rtt});
        }
    };

    void test_sequence_mismatch(sim::test::TestHarness& runner)
    {
        Fixture fixture{};

        push_actuator(fixture.channel, 5, 1000);
        runner.check(fixture.receive(6, 1000) == sim::hil::ReceiveResult::SequenceError, "sequence mismatch rejected");
        runner.check(fixture.transport.stats().sequence_errors >= 1, "sequence errors counted");
    }

    void test_echo_mismatch(sim::test::TestHarness& runner)
    {
        Fixture fixture{};

        push_actuator(fixture.channel, 5, 9999);
        runner.check(fixture.receive(5, 1000) == sim::hil::ReceiveResult::EchoMismatch,
                     "stale sim-timestamp echo rejected");
        runner.check(fixture.transport.stats().stale_packets >= 1, "stale packets counted");
    }

    void test_wrong_message_type(sim::test::TestHarness& runner)
    {
        Fixture                           fixture{};
        const FlightCore::HAL::SensorData sensor{};

        runner.check(sim::hil::send_sensor_frame(fixture.channel, sensor, 5), "sensor frame queued");
        runner.check(fixture.receive(5, 0) == sim::hil::ReceiveResult::Timeout,
                     "wrong message id dropped, then timeout");
        runner.check(fixture.transport.stats().messages_dropped >= 1, "wrong-type frame dropped");
    }

    void test_crc_corruption(sim::test::TestHarness& runner)
    {
        Fixture fixture{};

        push_actuator(fixture.channel, 5, 1000);
        std::array<std::uint8_t, FlightCore::Transport::kActuatorFrameSize> frame{};
        runner.check(fixture.channel.receiveBytes(frame) == frame.size(), "actuator frame drained");
        frame[FlightCore::Transport::kHeaderSize + 4] ^= 0xFFu;
        runner.check(fixture.channel.sendBytes(frame), "corrupt frame queued");
        runner.check(fixture.receive(5, 1000) == sim::hil::ReceiveResult::Timeout,
                     "CRC-corrupt frame rejected (timeout)");
        runner.check(fixture.transport.stats().messages_dropped >= 1, "corrupt frame counted as dropped");
    }

    void test_duplicate_sequence(sim::test::TestHarness& runner)
    {
        Fixture fixture{};

        push_actuator(fixture.channel, 4, 1000);
        push_actuator(fixture.channel, 5, 1000);
        runner.check(fixture.receive(5, 1000) == sim::hil::ReceiveResult::SequenceError,
                     "duplicate/stale earlier packet caught before the matching one");
    }
}

void run_transport_tests(sim::test::TestHarness& runner)
{
    test_sequence_mismatch(runner);
    test_echo_mismatch(runner);
    test_wrong_message_type(runner);
    test_crc_corruption(runner);
    test_duplicate_sequence(runner);
}

}
