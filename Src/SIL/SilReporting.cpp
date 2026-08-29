/*
Filename: Src/SIL/SilReporting.cpp
Description: Markdown report builders and SIL validation artifact orchestration.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilReportFormat;
import SilTypes;

namespace sim::sil {

namespace {

std::string scenario_body(const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;
    std::ostringstream out;
    out << "### " << record.name << "\n\n";
    out << "| Propriete | Valeur |\n|---|---|\n";
    out << "| Type de faulte | `" << fault_type_name(record.scenario.fault_type) << "` |\n";
    out << "| Debut (s) | " << format_seconds(record.scenario.start_time) << " |\n";
    out << "| Duree (s) | " << format_seconds(record.scenario.duration) << " |\n";
    out << "| Detectee a (s) | " << format_seconds(r.detection_time) << " |\n";
    out << "| Latence de detection (ms) | " << format_metric(1000.0 * r.detection_latency)
        << " |\n";
    out << "| Latence de reponse (ms) | " << format_metric(1000.0 * r.response_latency) << " |\n";
    out << "| Sante finale | `" << health_state_name(r.final_health) << "` |\n";
    out << "| Mode de surete final | `" << safety_mode_name(r.final_safety_mode) << "` |\n";
    out << "| Etat de mission final | `" << sim::control::mission_state_name(r.final_state)
        << "` |\n";
    out << "| Erreur de position max (m) | " << format_metric(r.max_position_error_m) << " |\n";
    out << "| Erreur d'altitude max (m) | " << format_metric(r.max_altitude_error_m) << " |\n";
    out << "| Altitude finale (m) | " << format_metric(r.final_altitude_m) << " |\n";
    out << "| Mission reussie | " << yes_no(r.mission_success) << " |\n";
    out << "| Verdict | " << (r.passed ? "**PASS**" : "**FAIL**") << " |\n\n";
    if (!r.failure_reason.empty()) {
        out << "> " << r.failure_reason << "\n\n";
    }
    return out.str();
}

std::string summary_table(const std::vector<ScenarioRecord>& records)
{
    std::ostringstream out;
    out << "| Scenario | Faulte | Detectee | Latence det. (ms) | Latence rep. (ms) | Mode "
           "final | Verdict |\n";
    out << "|---|---|---|---|---|---|---|\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        out << "| " << record.name << " | `" << fault_type_name(record.scenario.fault_type)
            << "` | " << yes_no(r.fault_detected) << " | "
            << format_metric(1000.0 * r.detection_latency) << " | "
            << format_metric(1000.0 * r.response_latency) << " | `"
            << safety_mode_name(r.final_safety_mode) << "` | " << (r.passed ? "PASS" : "FAIL")
            << " |\n";
    }
    return out.str();
}

std::string build_markdown(const std::vector<ScenarioRecord>& records)
{
    std::ostringstream out;
    out << "# Rapport de validation SIL\n\n";
    out << "Validation Software-in-the-Loop du systeme de fault injection.\n\n";
    out << "## Synthese\n\n" << summary_table(records) << "\n";
    out << "## Resultats detailles\n\n";
    for (const ScenarioRecord& record : records) {
        out << scenario_body(record);
    }
    return out.str();
}

}

std::string write_sil_report(const std::vector<ScenarioRecord>& records,
                             const SilReportOptions&            options)
{
    std::error_code ec;
    std::filesystem::create_directories(options.docs_dir, ec);
    std::string markdown;
    if (options.write_markdown) {
        markdown = build_markdown(records);
        write_file(options.docs_dir / "sil.md", markdown);
    }
    if (options.write_json) {
        write_file(options.docs_dir / "sil.json", json_payload(records));
    }
    if (options.write_csv) {
        write_file(options.docs_dir / "sil.csv", csv_payload(records));
    }
    return markdown;
}

}
