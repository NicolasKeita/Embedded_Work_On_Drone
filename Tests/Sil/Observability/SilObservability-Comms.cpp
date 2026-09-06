/*
Filename: Tests/Sil/Observability/SilObservability-Comms.cpp
Description: Communication statistics and verdict-independence tests.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservability;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

/*
OBS-006: the communication statistics aggregate matches the event trace.
*/
void comms_statistics_test(TestHarness& runner)
{
    runner.set_context("OBS-006");
    const SilConfig                             config{.duration_s = 32.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{ .start_time = 30.0, .fault_type = FaultType::CommunicationLoss
                                                };
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t sent = count_events(output.events, SilEventType::HeartbeatSent);
    const std::size_t dropped = count_events(output.events, SilEventType::HeartbeatDropped);
    const std::size_t timeouts = count_events(output.events, SilEventType::MessageTimeout);

    runner.check(output.result.comms.sent == sent, "comms.sent reflects the trace");
    runner.check(output.result.comms.dropped == dropped, "comms.dropped reflects the trace");
    runner.check(output.result.comms.delivered == sent - dropped, "delivered = sent - dropped");
    runner.check(output.result.comms.timeouts == timeouts && timeouts >= 1, "communication timeout observed");
    runner.check(output.result.comms.latency_min_s >= 0.0, "positive minimum latency");
}

/*
OBS-007: mission_success and test_verdict stay independent for an intentional
fault (mission aborted but verdict PASS).
*/
void verdict_independence_test(TestHarness& runner)
{
    runner.set_context("OBS-007");
    const SilConfig                             config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SimulationResult& result = outcome.value().result;
    const bool verdict = result.compute_verdict(true);

    runner.check(!result.mission_success, "mission not successful");
    runner.check(verdict, "verdict PASS for intentional fault");
    runner.check(result.mission_success != verdict, "verdict and mission success independent");
}

}
