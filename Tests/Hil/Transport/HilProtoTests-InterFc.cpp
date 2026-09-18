/*
Filename: Tests/Hil/Transport/HilProtoTests-InterFc.cpp
Description: HIL inter-FC protocol tests: verifies that the inter-FC status message and its
CRC survive a complete frame round trip through the byte-by-byte parser.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilProtoTests;

import std;

import InterFcLink;
import TestHarness;

namespace sim::test::hil {

namespace {
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

void run_proto_inter_fc_tests(sim::test::TestHarness& runner)
{
    test_inter_fc_status(runner);
}

}