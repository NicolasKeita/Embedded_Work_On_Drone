/*
Filename: Tests/Hil/Transport/HilProtoTests-Version.cpp
Description: HIL protocol version tests: rejects a legacy version even with a valid CRC,
then accepts the next current frame.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilProtoTests;

import std;

import HilProtocolCodec;
import HilProtocolParser;
import TestHarness;

namespace sim::test::hil {

namespace {
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
}

void run_proto_version_tests(sim::test::TestHarness& runner)
{
    test_protocol_version(runner);
}

}