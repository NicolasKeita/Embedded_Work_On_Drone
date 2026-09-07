/*
Filename: Tests/Sil/SilScenarios.cppm
Description: Interface of the deterministic SIL test suite (nominal and fault-injection scenarios);
observability tests live in the SilObservability module. Exposes the engine-level scenario
catalog so a runner can execute one scenario by its standardised ID (NOMINAL-001,
FAULT_INJECTOR-001..005).
Exports:
    struct SilScenarioEntry,
    sil_scenarios(),
    find_sil_scenario(),
    run_sil_scenario(),
    run_all_sil_scenarios()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilScenarios;

import std;

import SilReporting;
import SilRunner;
import SilRunnerContext;
import TestHarness;

export namespace sim::test::sil {

/*
One engine-level SIL scenario of the suite: its standardised ID, a short
description and the runner function producing the structured output and the
report record.
*/
struct SilScenarioEntry {
    std::string_view id;
    std::string_view description;
    void (*run)(sim::test::TestHarness&, sim::sil::SilRunOutput&, sim::sil::ScenarioRecord&);
};

/* Catalog of the engine-level SIL scenarios, in report order. */
[[nodiscard]] std::span<const SilScenarioEntry> sil_scenarios() noexcept;

/*
Resolves one engine-level SIL scenario by its standardised ID; returns nullptr
when the ID is unknown.
*/
[[nodiscard]] const SilScenarioEntry* find_sil_scenario(std::string_view id) noexcept;

/*
Runs a single engine-level SIL scenario by ID; returns false when the ID is
unknown. The scenario verdict is accumulated in the harness and the periodic
telemetry table is streamed to stdout at telemetry_report_interval_s.
*/
bool run_sil_scenario(std::string_view id, sim::test::TestHarness& runner,
                      std::float64_t telemetry_report_interval_s = 1.0);

/* Runs the six scenarios, the observability suite and the report artifacts. */
void run_all_sil_scenarios(TestHarness& runner);

}
