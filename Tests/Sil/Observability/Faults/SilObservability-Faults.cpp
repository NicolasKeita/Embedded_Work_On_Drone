/*
Filename: Tests/Sil/Observability/Faults/SilObservability-Faults.cpp
Description: FC1 failure events, detection latency and state transition tests.

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
using sim::sil::FailureMode;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

namespace {

    /* Checks the injection, failure, detection and supervision events of one FC1 failure trace. */
    void check_fc1_failure_events(TestHarness& runner, const SilRunOutput& output, const SilConfig& config)
    {
        const SilEvent* injected = find_first(output.events, SilEventType::FaultInjected);
        const SilEvent* failure = find_first(output.events, SilEventType::FCFailure);
        const SilEvent* detected = find_first(output.events, SilEventType::FaultDetected);

        runner.check(injected != nullptr, "FAULT_INJECTED event present");
        runner.check(failure != nullptr, "FC_FAILURE event present");
        runner.check(detected != nullptr, "FAULT_DETECTED event present");
        if (injected == nullptr || detected == nullptr || failure == nullptr) {
            return;
        }
        runner.check(std::abs(injected->timestamp - 30.0) <= config.dt, "FAULT_INJECTED timestamped at injection");
        runner.check(injected->detail == sim::sil::failure_mode_name(FailureMode::FC1_UNAVAILABLE),
                     "FAULT_INJECTED carries the failure mode");
        runner.check(injected->reason.find("permanently") != std::string_view::npos,
                     "FAULT_INJECTED specifies the permanent character");
        runner.check(detected->timestamp >= 30.0 && output.result.fault_detected, "detection after injection");
        runner.check(output.result.detection_latency >= 0.0 && output.result.detection_latency <= 0.30,
                     "detection latency within 300 ms");
        runner.check(std::abs(output.result.detection_latency
                              - (output.result.detection_time - output.result.fault_injected_time)) < 1.0e-9,
                     "detection_latency = detection_time - fault_injected_time");
        runner.check(output.result.supervision_triggered
                         && output.result.supervision_trigger_time <= output.result.detection_time + 1.0e-9,
                     "supervision triggered no later than detection");
        runner.check(output.result.safety_response_time >= output.result.detection_time,
                     "safety response after detection");
    }
}

/*
OBS-003/004: an FC1 failure produces injection, FC failure, supervision and
detection events, and the detection latency is correctly derived.
*/
void fc1_failure_events_test(TestHarness& runner)
{
    runner.set_context("OBS-003");
    const SilConfig                             config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{
        .start_time = 30.0, .failure_mode = FailureMode::FC1_UNAVAILABLE};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }
    check_fc1_failure_events(runner, outcome.value(), config);
}

/*
OBS-005: mission and safety state transitions are recorded with the previous,
new state and the reason that drove the change.
*/
void state_transitions_test(TestHarness& runner)
{
    runner.set_context("OBS-005");
    const SilConfig                             config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{
        .start_time = 30.0, .failure_mode = FailureMode::FC1_UNAVAILABLE};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t mission_tx = count_events(output.events, SilEventType::MissionStateTransition);
    const std::size_t safety_tx = count_events(output.events, SilEventType::SafetyStateTransition);

    runner.check(mission_tx >= 2, "mission transitions observed");
    runner.check(safety_tx >= 1, "safety transition observed");

    bool has_documented_transition = false;
    for (const SilEvent& event : output.events) {
        if (event.type != SilEventType::MissionStateTransition) {
            continue;
        }
        has_documented_transition = !event.previous_state.empty() && !event.new_state.empty() && !event.reason.empty();
        if (has_documented_transition) {
            break;
        }
    }
    runner.check(has_documented_transition, "transitions carry previous/new/reason");
}

}
