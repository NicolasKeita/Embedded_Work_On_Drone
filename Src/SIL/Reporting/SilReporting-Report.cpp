/*
Filename: Src/SIL/Reporting/SilReporting-Report.cpp
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

namespace {

/*
Writes one detailed scenario section of the Markdown report.
*/
void write_scenario_section(std::ostream& out, const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;

    out << "### " << record.name << "\n\n";
    out << "| Propriete | Valeur |\n|---|---|\n";
    out << "| Type de faulte | `" << fault_type_name(record.scenario.fault_type) << "` |\n";
    out << "| Debut (s) | ";
    write_seconds(out, record.scenario.start_time);
    out << " |\n";
    out << "| Duree (s) | ";
    write_seconds(out, record.scenario.duration);
    out << " |\n";
    out << "| Detectee a (s) | ";
    write_seconds(out, r.detection_time);
    out << " |\n";
    out << "| Latence de detection (ms) | ";
    write_metric(out, 1000.0 * r.detection_latency);
    out << " |\n";
    out << "| Latence de reponse (ms) | ";
    write_metric(out, 1000.0 * r.response_latency);
    out << " |\n";
    out << "| Sante finale | `" << health_state_name(r.final_health) << "` |\n";
    out << "| Mode de surete final | `" << safety_mode_name(r.final_safety_mode) << "` |\n";
    out << "| Etat de mission final | `" << sim::control::mission_state_name(r.final_state) << "` |\n";
    out << "| Erreur de position max (m) | ";
    write_metric(out, r.max_position_error_m);
    out << " |\n";
    out << "| Erreur d'altitude max (m) | ";
    write_metric(out, r.max_altitude_error_m);
    out << " |\n";
    out << "| Altitude finale (m) | ";
    write_metric(out, r.final_altitude_m);
    out << " |\n";
    out << "| Mission reussie | " << yes_no(r.mission_success) << " |\n";
    out << "| Verdict | " << (r.passed ? "**PASS**" : "**FAIL**") << " |\n\n";
    if (r.mission_aborted) {
        out << "> Safe mode engage : " << fault_domain_name(r.first_fault_domain) << "\n\n";
    }
}

/*
Writes the summary table of the Markdown report.
*/
void write_summary_table(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "| Scenario | Faulte | Detectee | Latence det. (ms) | Latence rep. (ms) | Mode "
           "final | Verdict |\n";
    out << "|---|---|---|---|---|---|---|\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        out << "| " << record.name << " | `" << fault_type_name(record.scenario.fault_type)
            << "` | " << yes_no(r.fault_detected) << " | ";
        write_metric(out, 1000.0 * r.detection_latency);
        out << " | ";
        write_metric(out, 1000.0 * r.response_latency);
        out << " | `" << safety_mode_name(r.final_safety_mode) << "` | " << (r.passed ? "PASS" : "FAIL")
            << " |\n";
    }
}

}

/*
Writes the full Markdown report into the given stream.
*/
void write_markdown_report(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "# Rapport de validation SIL\n\n";
    out << "Validation Software-in-the-Loop du systeme de fault injection.\n\n";
    out << "## Synthese\n\n";
    write_summary_table(out, records);
    out << "\n";
    out << "## Resultats detailles\n\n";
    for (const ScenarioRecord& record : records) {
        write_scenario_section(out, record);
    }
}

}
