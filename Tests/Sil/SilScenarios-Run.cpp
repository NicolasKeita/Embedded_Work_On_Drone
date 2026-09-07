/*
Filename: Tests/Sil/SilScenarios-Run.cpp
Description: Single-scenario dispatch and suite orchestration of the SIL test suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import SilObservability;
import SilObservabilityTelemetry;
import SilReporting;
import TestHarness;

namespace sim::test::sil {

using sim::sil::ScenarioRecord;
using sim::sil::SilError;
using sim::sil::SilRunOutput;

bool run_sil_scenario(std::string_view id, TestHarness& runner, std::float64_t telemetry_report_interval_s)
{
    const SilScenarioEntry* entry = find_sil_scenario(id);

    if (entry == nullptr) {
        return false;
    }

    SilRunOutput   output{};
    ScenarioRecord record{};
    entry->run(runner, output, record);

    std::cout << "\n--- Mission telemetry (period " << telemetry_report_interval_s << " s) ---\n";
    sim::sil::write_telemetry_table(std::cout, record.telemetry, telemetry_report_interval_s);
    return true;
}

/* Runs the six scenarios, the observability suite and the report artifacts. */
void run_all_sil_scenarios(TestHarness& runner)
{
    std::array<SilRunOutput, 6>             outputs{};
    std::array<ScenarioRecord, 6>           records{};
    const std::span<const SilScenarioEntry> entries = sil_scenarios();

    for (std::size_t index = 0; index < entries.size(); ++index) {
        entries[index].run(runner, outputs[index], records[index]);
    }

    run_observability_scenarios(runner);
    run_telemetry_scenarios(runner);

    std::cout << "\n=== SIL report generation (docs/validation/data) ===" << std::endl;
    const std::expected<void, sim::sil::ReportError> outcome = sim::sil::write_sil_report(records);
    if (!outcome.has_value()) {
        runner.check(false, "SIL report generation failed");
        return;
    }
    std::cout << "Artifacts generated: sil.md, json, csv, trace jsonl, telemetry csv, truth csv" << std::endl;
    sim::sil::write_markdown_report(std::cout, records);
}

}
