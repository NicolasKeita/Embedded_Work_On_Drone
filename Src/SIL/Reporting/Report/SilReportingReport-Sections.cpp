/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Sections.cpp
Description: Markdown row writers of the detailed scenario sections.

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

/*
Writes the fault identity rows of one scenario section.
*/
void write_scenario_fault_rows(std::ostream& out, const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;

    out << "| Failure mode | `" << failure_mode_name(record.scenario.failure_mode) << "` |\n";
    out << "| Start (s) | ";
    write_seconds(out, record.scenario.start_time);
    out << " |\n";
    out << "| Duration (s) | ";
    write_seconds(out, record.scenario.duration);
    out << " |\n";
    out << "| Detected at (s) | ";
    write_seconds(out, r.detection_time);
    out << " |\n";
    out << "| Detection latency (ms) | ";
    write_metric(out, 1000.0 * r.detection_latency);
    out << " |\n";
    out << "| Response latency (ms) | ";
    write_metric(out, 1000.0 * r.response_latency);
    out << " |\n";
}

/*
Writes the safety, aircraft, communication and verdict rows of one section.
*/
void write_scenario_outcome_rows(std::ostream& out, const SimulationResult& r)
{
    out << "| Final health | `" << health_state_name(r.final_health) << "` |\n";
    out << "| Final safety mode | `" << safety_mode_name(r.final_safety_mode) << "` |\n";
    out << "| Final mission state | `" << sim::control::mission_state_name(r.final_state) << "` |\n";
    out << "| Max position error (m) | ";
    write_metric(out, r.max_position_error_m);
    out << " |\n";
    out << "| Max altitude error (m) | ";
    write_metric(out, r.max_altitude_error_m);
    out << " |\n";
    out << "| Final altitude (m) | ";
    write_metric(out, r.final_altitude_m);
    out << " |\n";
    out << "| Supervision triggered | " << yes_no(r.supervision_triggered) << " |\n";
    out << "| Messages sent / received / lost | " << r.comms.sent << " / " << r.comms.delivered << " / "
        << r.comms.dropped << " |\n";
    out << "| Last received sequence | " << r.comms.last_sequence << " |\n";
    out << "| Communication timeouts | " << r.comms.timeouts << " |\n";
    out << "| Min / mean / max latency (ms) | ";
    write_metric(out, 1000.0 * r.comms.latency_min_s);
    out << " / ";
    write_metric(out, 1000.0 * r.comms.latency_mean_s);
    out << " / ";
    write_metric(out, 1000.0 * r.comms.latency_max_s);
    out << " |\n";
    out << "| Mission successful | " << yes_no(r.mission_success) << " |\n";
    out << "| Verdict | " << (r.test_verdict ? "**PASS**" : "**FAIL**") << " |\n\n";
    out << "> Verdict: " << verdict_reason(r) << "\n\n";
}

}
