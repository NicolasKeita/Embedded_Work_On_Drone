/*
Filename: Tests/Sil/Observability/SilObservability.cppm
Description: Interface of the SIL observability suite (heartbeat trace, fault events, statistics, verdicts).
Exports:
    run_traced(),
    count_events(),
    find_first(),
    find_first_after(),
    run_observability_scenarios()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilObservability;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

export namespace sim::test::sil {

// Shared trace helpers reused by the telemetry observability tests.
std::expected<sim::sil::SilRunOutput, sim::sil::SilError> run_traced(const sim::sil::SilConfig& config,
                                                                     const sim::sil::FaultScenario& scenario);
std::size_t count_events(std::span<const sim::sil::SilEvent> events, sim::sil::SilEventType type);
const sim::sil::SilEvent* find_first(std::span<const sim::sil::SilEvent> events, sim::sil::SilEventType type);
const sim::sil::SilEvent* find_first_after(std::span<const sim::sil::SilEvent> events,
                                           sim::sil::SilEventType type, std::size_t index);

// Observability suite: heartbeat trace, fault events, statistics, verdicts.
void run_observability_scenarios(TestHarness& runner);

}

namespace sim::test::sil {

// Per-test scenario checks (split across the implementation files).
void heartbeat_trace_test(TestHarness& runner);
void dropped_heartbeats_test(TestHarness& runner);
void fc1_failure_events_test(TestHarness& runner);
void state_transitions_test(TestHarness& runner);
void comms_statistics_test(TestHarness& runner);
void verdict_independence_test(TestHarness& runner);
void structured_fields_test(TestHarness& runner);
void logging_neutrality_test(TestHarness& runner);
void trace_reconstruction_test(TestHarness& runner);

}