/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Markdown.cpp
Description: Markdown report writers for the SIL validation artifacts.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilTypes;

namespace sim::sil {

void write_scenario_fault_rows(std::ostream& out, const ScenarioRecord& record);
void write_scenario_outcome_rows(std::ostream& out, const SimulationResult& result);
void write_scenario_telemetry_sections(std::ostream&         out,
                                       const ScenarioRecord& record,
                                       std::float64_t        telemetry_report_interval_s);

/*
Writes one detailed scenario section (concise, no raw trace) followed by the
periodic telemetry, event and post-fault telemetry subsections.
*/
void write_scenario_section(std::ostream&         out,
                            const ScenarioRecord& record,
                            std::float64_t        telemetry_report_interval_s)
{
    const SimulationResult& r = record.result;

    out << "### [" << record.name << "][SIL]\n\n";
    out << "| Property | Value |\n|---|---|\n";
    write_scenario_fault_rows(out, record);
    write_scenario_outcome_rows(out, r);
    write_scenario_telemetry_sections(out, record, telemetry_report_interval_s);
}

/*
Writes the summary table of the Markdown report.
*/
void write_summary_table(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "| Scenario | Failure mode | Detected | Det. latency (ms) | Resp. latency (ms) | Mission state "
           "| Verdict |\n";
    out << "|---|---|---|---|---|---|---|\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        out << "| [" << record.name << "][SIL] | `" << failure_mode_name(record.scenario.failure_mode)
            << "` | " << yes_no(r.fault_detected) << " | ";
        write_metric(out, 1000.0 * r.detection_latency);
        out << " | ";
        write_metric(out, 1000.0 * r.response_latency);
        out << " | `" << sim::control::mission_state_name(r.final_state) << "` | " << (r.test_verdict ? "PASS" : "FAIL")
            << " |\n";
    }
}

/*
Writes the full Markdown report into the given stream, downsampling the
telemetry tables at the requested report interval.
*/
void write_markdown_report(std::ostream&                   out,
                           std::span<const ScenarioRecord> records,
                           std::float64_t                  telemetry_report_interval_s)
{
    out << "# SIL Validation Report\n\n";
    out << "Software-in-the-Loop validation of the fault injection system (target: SIL).\n\n";
    out << "## Summary\n\n";
    write_summary_table(out, records);
    out << "\n";
    out << "## Detailed results\n\n";
    for (const ScenarioRecord& record : records) {
        write_scenario_section(out, record, telemetry_report_interval_s);
    }
}

}
