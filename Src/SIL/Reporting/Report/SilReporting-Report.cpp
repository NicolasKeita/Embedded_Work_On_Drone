/*
Filename: Src/SIL/Reporting/Report/SilReporting-Report.cpp
Description: Markdown report writers for the SIL validation artifacts.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilTypes;

namespace sim::sil {

void write_scenario_fault_rows(std::ostream& out, const ScenarioRecord& record);
void write_scenario_outcome_rows(std::ostream& out, const SimulationResult& result);
void write_scenario_telemetry_sections(std::ostream& out, const ScenarioRecord& record,
                                       double telemetry_report_interval_s);

/*
Writes one detailed scenario section (concise, no raw trace) followed by the
periodic telemetry, event and post-fault telemetry subsections.
*/
void write_scenario_section(std::ostream& out, const ScenarioRecord& record,
                            double telemetry_report_interval_s)
{
    const SimulationResult& r = record.result;

    out << "### " << record.name << "\n\n";
    out << "| Propriete | Valeur |\n|---|---|\n";
    write_scenario_fault_rows(out, record);
    write_scenario_outcome_rows(out, r);
    write_scenario_telemetry_sections(out, record, telemetry_report_interval_s);
}

/*
Writes the summary table of the Markdown report.
*/
void write_summary_table(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "| Scenario | Faulte | Detectee | Latence det. (ms) | Latence rep. (ms) | Etat mission "
           "| Verdict |\n";
    out << "|---|---|---|---|---|---|---|\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        out << "| " << record.name << " | `" << fault_type_name(record.scenario.fault_type)
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
void write_markdown_report(std::ostream& out, std::span<const ScenarioRecord> records,
                           double telemetry_report_interval_s)
{
    out << "# Rapport de validation SIL\n\n";
    out << "Validation Software-in-the-Loop du systeme de fault injection.\n\n";
    out << "## Synthese\n\n";
    write_summary_table(out, records);
    out << "\n";
    out << "## Resultats detailles\n\n";
    for (const ScenarioRecord& record : records) {
        write_scenario_section(out, record, telemetry_report_interval_s);
    }
}

}