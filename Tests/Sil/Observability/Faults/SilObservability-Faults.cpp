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
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

/*
OBS-003/004: an FC1 failure produces injection, FC failure, watchdog and
detection events, and the detection latency is correctly derived.
*/
void fc1_failure_events_test(TestHarness& runner)
{
    const SilConfig                             config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-003 : moteur SIL en echec");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const SilEvent* injected = find_first(output.events, SilEventType::FaultInjected);
    const SilEvent* failure = find_first(output.events, SilEventType::FCFailure);
    const SilEvent* detected = find_first(output.events, SilEventType::FaultDetected);

    runner.check(injected != nullptr, "OBS-003 : evenement FAULT_INJECTED present");
    runner.check(failure != nullptr, "OBS-003 : evenement FC_FAILURE present");
    runner.check(detected != nullptr, "OBS-003 : evenement FAULT_DETECTED present");
    if (injected == nullptr || detected == nullptr || failure == nullptr) {
        return;
    }
    runner.check(std::abs(injected->timestamp - 30.0) <= config.dt, "OBS-003 : FAULT_INJECTED horodate a l'injection");
    runner.check(injected->detail == sim::sil::fault_type_name(FaultType::FC1Failure),
                 "OBS-003 : FAULT_INJECTED porte le type de faulte");
    runner.check(injected->reason.find("permanently") != std::string_view::npos,
                 "OBS-003 : FAULT_INJECTED precise le caractere permanent");
    runner.check(detected->timestamp >= 30.0 && output.result.fault_detected, "OBS-003 : detection apres l'injection");
    runner.check(output.result.detection_latency >= 0.0 && output.result.detection_latency <= 0.30,
                 "OBS-004 : latence de detection dans la limite de 300 ms");
    runner.check(std::abs(output.result.detection_latency
                          - (output.result.detection_time - output.result.fault_injected_time)) < 1.0e-9,
                 "OBS-004 : detection_latency = detection_time - fault_injected_time");
    runner.check(output.result.watchdog_triggered
                     && output.result.watchdog_trigger_time <= output.result.detection_time + 1.0e-9,
                 "OBS-004 : watchdog declenche au plus tard a la detection");
    runner.check(output.result.safety_response_time >= output.result.detection_time,
                 "OBS-004 : reponse de surete apres detection");
}

/*
OBS-005: mission and safety state transitions are recorded with the previous,
new state and the reason that drove the change.
*/
void state_transitions_test(TestHarness& runner)
{
    const SilConfig                             config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-005 : moteur SIL en echec");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t mission_tx = count_events(output.events, SilEventType::MissionStateTransition);
    const std::size_t safety_tx = count_events(output.events, SilEventType::SafetyStateTransition);

    runner.check(mission_tx >= 2, "OBS-005 : transitions mission observees");
    runner.check(safety_tx >= 1, "OBS-005 : transition de surete observee");

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
    runner.check(has_documented_transition, "OBS-005 : transitions portent previous/new/reason");
}

}
