/*
Filename: Tests/Sil/Observability/SilObservability-Logging.cpp
Description: Structured SimulationResult fields and logging neutrality tests.

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
OBS-008: the structured SimulationResult carries the full fault chain and the
aircraft/metric aggregates populated at every run.
*/
void structured_fields_test(TestHarness& runner)
{
    runner.set_context("OBS-008");
    const SilConfig                             config{.duration_s = 30.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{.start_time = 20.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SimulationResult& result = outcome.value().result;

    runner.check(result.fault_injected_time >= 20.0 && result.fault_detected, "injection chain detected");
    runner.check(result.detection_time >= result.fault_injected_time, "detection after injection");
    runner.check(std::abs(result.detection_latency
                          - (result.detection_time - result.fault_injected_time)) < 1.0e-9,
                 "detection latency consistent");
    runner.check(result.response_latency >= 0.0 && result.safety_response_time >= result.detection_time,
                 "response latency consistent");
    runner.check(std::isfinite(result.final_altitude_m) && result.max_altitude_error_m >= 0.0,
                 "aircraft fields populated");
    runner.check(result.max_position_error_m >= 0.0 && result.mean_altitude_error_m >= 0.0,
                 "error aggregates populated");
    runner.check(outcome.value().telemetry.size() > 0 && outcome.value().events.size() > 10,
                 "structured telemetry and trace present");
}

/*
OBS-009: lowering the trace verbosity never changes the simulation result, only
the richness of the recorded event trace (observational neutrality).
*/
void logging_neutrality_test(TestHarness& runner)
{
    runner.set_context("OBS-009");
    const FaultScenario                         scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const SilConfig                             info_cfg{.duration_s = 35.0};
    const SilConfig                             trace_cfg{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const std::expected<SilRunOutput, SilError> info = run_traced(info_cfg, scenario);
    const std::expected<SilRunOutput, SilError> trace = run_traced(trace_cfg, scenario);

    if (!info.has_value() || !trace.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SimulationResult& i = info.value().result;
    const SimulationResult& t = trace.value().result;

    runner.check(i.mission_success == t.mission_success, "mission success independent");
    runner.check(i.final_state == t.final_state, "terminal state independent");
    runner.check(i.detection_latency == t.detection_latency, "identical detection latency");
    runner.check(i.comms.sent == t.comms.sent && i.comms.dropped == t.comms.dropped, "identical comms statistics");
    runner.check(i.max_altitude_error_m == t.max_altitude_error_m, "identical aircraft metrics");
    runner.check(count_events(info.value().events, SilEventType::HeartbeatSent)
                 < count_events(trace.value().events, SilEventType::HeartbeatSent),
                 "verbose trace richer without changing the result");
}

}
