/*
Filename: Src/SIL/Reporting/Report/SilReporting-Report-Sections.cpp
Description: Markdown row writers of the detailed scenario sections.

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

/*
Writes the fault identity rows of one scenario section.
*/
void write_scenario_fault_rows(std::ostream& out, const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;

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
}

/*
Writes the safety, aircraft, communication and verdict rows of one section.
*/
void write_scenario_outcome_rows(std::ostream& out, const SimulationResult& r)
{
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
    out << "| Watchdog declenche | " << yes_no(r.watchdog_triggered) << " |\n";
    out << "| Messages envoyes / recus / perdus | " << r.comms.sent << " / " << r.comms.delivered << " / "
        << r.comms.dropped << " |\n";
    out << "| Derniere sequence recue | " << r.comms.last_sequence << " |\n";
    out << "| Timeouts de communication | " << r.comms.timeouts << " |\n";
    out << "| Latence min / moyenne / max (ms) | ";
    write_metric(out, 1000.0 * r.comms.latency_min_s);
    out << " / ";
    write_metric(out, 1000.0 * r.comms.latency_mean_s);
    out << " / ";
    write_metric(out, 1000.0 * r.comms.latency_max_s);
    out << " |\n";
    out << "| Mission reussie | " << yes_no(r.mission_success) << " |\n";
    out << "| Verdict | " << (r.test_verdict ? "**PASS**" : "**FAIL**") << " |\n\n";
    out << "> Verdict : " << verdict_reason(r) << "\n\n";
}

}