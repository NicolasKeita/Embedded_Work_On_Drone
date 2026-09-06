/*
Filename: Tests/Hil/HilTests-Protocol.cpp
Description: HIL protocol tests : sequence validation, message-type/echo/length checks,
duplicate/stale and CRC-corrupt packet handling at the transport layer.

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
    FlightCore::HAL::ActuatorCommands sample_commands(std::uint64_t stamp, std::uint32_t seq) noexcept
    {
        return FlightCore::HAL::ActuatorCommands{
            .timestamp_us = stamp,
            .wing_rpm_cmd = 1000.0f + static_cast<std::float32_t>(seq),
            .left_servo_rad = 0.05f,
            .right_servo_rad = -0.05f,
            .aux_actuator_cmd = 0.0f,
            .mode_flags = 2,
        };
    }

    void push_actuator(FlightCore::Sim::LoopbackTransport& channel, std::uint16_t seq, std::uint64_t echo_us)
    {
        const FlightCore::HAL::ActuatorCommands cmds = sample_commands(7, seq);
        const FlightCore::Transport::ActuatorDiagnostics diag{};
        const auto payload = FlightCore::Transport::makeActuatorPayload(cmds, echo_us, diag);
        const auto frame = FlightCore::Transport::encodeActuatorFrame(payload, seq);
        channel.sendBytes(frame);
    }

    sim::hil::ReceiveResult receive(sim::hil::HilTransport& transport, sim::hil::FastClock& clock,
                                     std::uint16_t expected_seq, std::uint64_t expected_echo)
    {
        FlightCore::HAL::ActuatorCommands out{};
        FlightCore::Transport::ActuatorDiagnostics diag{};
        std::int64_t rtt = 0;
        return transport.receiveActuator(clock, expected_seq, expected_echo, 0, clock.nowUs() + 50000, out, diag, rtt);
    }
}

void run_protocol_tests(sim::test::TestHarness& runner)
{
    {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport transport{&channel};
        sim::hil::FastClock clock;
        push_actuator(channel, 5, 1000);
        runner.check(receive(transport, clock, 6, 1000) == sim::hil::ReceiveResult::SequenceError,
                     "protocol : sequence mismatch rejected");
        runner.check(transport.stats().sequence_errors >= 1, "protocol : sequence errors counted");
    }

    {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport transport{&channel};
        sim::hil::FastClock clock;
        push_actuator(channel, 5, 9999);
        runner.check(receive(transport, clock, 5, 1000) == sim::hil::ReceiveResult::EchoMismatch,
                     "protocol : stale sim-timestamp echo rejected");
        runner.check(transport.stats().stale_packets >= 1, "protocol : stale packets counted");
    }

    {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport transport{&channel};
        sim::hil::FastClock clock;
        const FlightCore::HAL::SensorData sensor{};
        sim::hil::send_sensor_frame(channel, sensor, 5);
        runner.check(receive(transport, clock, 5, 0) == sim::hil::ReceiveResult::Timeout,
                     "protocol : wrong message id dropped, then timeout");
        runner.check(transport.stats().messages_dropped >= 1, "protocol : wrong-type frame dropped");
    }

    {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport transport{&channel};
        sim::hil::FastClock clock;
        push_actuator(channel, 5, 1000);
        std::array<std::uint8_t, FlightCore::Transport::kActuatorFrameSize> frame{};
        channel.receiveBytes(frame);
        frame[FlightCore::Transport::kHeaderSize + 4] ^= 0xFFu;
        channel.sendBytes(frame);
        runner.check(receive(transport, clock, 5, 1000) == sim::hil::ReceiveResult::Timeout,
                     "protocol : CRC-corrupt frame rejected (timeout)");
        runner.check(transport.stats().messages_dropped >= 1, "protocol : corrupt frame counted as dropped");
    }

    {
        FlightCore::Sim::LoopbackTransport channel;
        sim::hil::HilTransport transport{&channel};
        sim::hil::FastClock clock;
        push_actuator(channel, 4, 1000);
        push_actuator(channel, 5, 1000);
        runner.check(receive(transport, clock, 5, 1000) == sim::hil::ReceiveResult::SequenceError,
                     "protocol : duplicate/stale earlier packet caught before the matching one");
    }
}

}
