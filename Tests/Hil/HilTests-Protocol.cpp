/*
Filename: Tests/Hil/HilTests-Protocol.cpp
Description: HIL and inter-FC protocol tests covering status, sequence, echo, message
type and CRC validation at the transport layer.

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
import InterFcLink;
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

    /* Verifies that the inter-FC status and its CRC survive a complete frame round trip. */
    void test_inter_fc_status(sim::test::TestHarness& runner)
    {
        const FlightCore::InterFc::Message status{
            .kind = FlightCore::InterFc::MessageKind::Status,
            .sequence = 42,
            .state = FlightCore::InterFc::NodeState::Safe,
            .detection = FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout,
        };
        const FlightCore::InterFc::FrameCodec::Frame frame = FlightCore::InterFc::FrameCodec::encode(status);
        FlightCore::InterFc::FrameParser parser{};
        std::optional<FlightCore::InterFc::Message> decoded{};

        for (const std::uint8_t byte : frame) {
            const std::optional<FlightCore::InterFc::Message> candidate = parser.process(byte);
            if (candidate.has_value()) {
                decoded = candidate;
            }
        }
        runner.check(decoded.has_value(), "inter-FC status frame accepted");
        runner.check(decoded.has_value() && decoded->kind == FlightCore::InterFc::MessageKind::Status,
                     "inter-FC status kind preserved");
        runner.check(decoded.has_value() && decoded->sequence == 42,
                     "inter-FC status sequence preserved");
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
    test_sequence_mismatch(runner);
    test_echo_mismatch(runner);
    test_wrong_message_type(runner);
    test_crc_corruption(runner);
    test_duplicate_sequence(runner);
    test_inter_fc_status(runner);
}

}
