/*
Filename: Tests/Sil/Observability/SilObservability-Support.cpp
Description: Shared helpers and orchestration of the SIL observability suite.

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
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilRunOutput;
using sim::sil::SILRunner;

/*
Runs one traced scenario with the given configuration.
*/
std::expected<SilRunOutput, SilError> run_traced(const SilConfig& config, const FaultScenario& scenario)
{
    const std::array<FaultScenario, 1> scenarios{scenario};

    return SILRunner{config}.run(scenarios);
}

/*
Counts the events of one type in a trace.
*/
std::size_t count_events(std::span<const SilEvent> events, SilEventType type)
{
    std::size_t count = 0;

    for (const SilEvent& event : events) {
        count += event.type == type ? 1u : 0u;
    }
    return count;
}

/*
Returns the first event of one type, or nullptr.
*/
const SilEvent* find_first(std::span<const SilEvent> events, SilEventType type)
{
    for (const SilEvent& event : events) {
        if (event.type == type) {
            return &event;
        }
    }
    return nullptr;
}

/*
Returns the first event of one type after the given index, or nullptr.
*/
const SilEvent* find_first_after(std::span<const SilEvent> events, SilEventType type, std::size_t index)
{
    for (std::size_t i = index + 1; i < events.size(); ++i) {
        if (events[i].type == type) {
            return &events[i];
        }
    }
    return nullptr;
}

/*
Runs the whole observability suite.
*/
void run_observability_scenarios(TestHarness& runner)
{
    std::cout << "\n=== Suite observabilite SIL ===" << std::endl;

    heartbeat_trace_test(runner);
    dropped_heartbeats_test(runner);
    fc1_failure_events_test(runner);
    fault_metadata_test(runner);
    sensor_fault_metadata_test(runner);
    state_transitions_test(runner);
    comms_statistics_test(runner);
    verdict_independence_test(runner);
    structured_fields_test(runner);
    logging_neutrality_test(runner);
    trace_reconstruction_test(runner);
}

}
