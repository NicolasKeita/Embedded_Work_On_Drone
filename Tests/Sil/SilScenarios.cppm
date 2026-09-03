/*
Filename: Tests/Sil/SilScenarios.cppm
Description: Interface of the deterministic SIL test suite (SIL-001 to SIL-005) and observability tests.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilScenarios;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

export namespace sim::test::sil {

void run_all_sil_scenarios(TestHarness& runner);

// Observability suite: heartbeat trace, fault events, statistics, verdicts.
void run_observability_scenarios(TestHarness& runner);

}

namespace sim::test::sil {

// Shared observability helpers (split across the implementation files).
std::expected<sim::sil::SilRunOutput, sim::sil::SilError> run_traced(const sim::sil::SilConfig& config,
                                                                     const sim::sil::FaultScenario& scenario);
std::size_t count_events(std::span<const sim::sil::SilEvent> events, sim::sil::SilEventType type);
const sim::sil::SilEvent* find_first(std::span<const sim::sil::SilEvent> events, sim::sil::SilEventType type);
const sim::sil::SilEvent* find_first_after(std::span<const sim::sil::SilEvent> events,
                                           sim::sil::SilEventType type, std::size_t index);
void heartbeat_trace_test(TestHarness& runner);
void dropped_heartbeats_test(TestHarness& runner);
void fc1_failure_events_test(TestHarness& runner);
void state_transitions_test(TestHarness& runner);
void comms_statistics_test(TestHarness& runner);
void verdict_independence_test(TestHarness& runner);
void structured_fields_test(TestHarness& runner);
void logging_neutrality_test(TestHarness& runner);
void trace_reconstruction_test(TestHarness& runner);

// Telemetry-path suite (sensor chain observability).
void telemetry_sampling_test(TestHarness& runner);
void telemetry_sensor_fault_path_test(TestHarness& runner);
void telemetry_sensor_hold_and_clear_test(TestHarness& runner);
void telemetry_rate_neutrality_test(TestHarness& runner);

}
