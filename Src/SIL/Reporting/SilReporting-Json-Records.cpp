/*
Filename: Src/SIL/Reporting/SilReporting-Json-Records.cpp
Description: JSON record field writers for the SIL results export.

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
Writes the identity and mission fields of one JSON record.
*/
void write_record_mission(std::ostream& out, const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;

    out << "    \"name\": \"";
    write_json_escaped(out, record.name);
    out << "\",\n";
    out << "    \"fault_type\": \"" << fault_type_name(record.scenario.fault_type) << "\",\n";
    out << "    \"start_time\": ";
    write_seconds(out, record.scenario.start_time);
    out << ",\n";
    out << "    \"duration\": ";
    write_seconds(out, record.scenario.duration);
    out << ",\n";
    out << "    \"mission_success\": " << (r.mission_success ? "true" : "false") << ",\n";
    out << "    \"mission_aborted\": " << (r.mission_aborted ? "true" : "false") << ",\n";
    out << "    \"final_state\": \"";
    write_json_escaped(out, sim::control::mission_state_name(r.final_state));
    out << "\",\n";
    out << "    \"final_health\": \"" << health_state_name(r.final_health) << "\",\n";
    out << "    \"final_safety_mode\": \"" << safety_mode_name(r.final_safety_mode) << "\",\n";
    out << "    \"fault_detected\": " << (r.fault_detected ? "true" : "false") << ",\n";
}

namespace {

/*
Writes the failure_reason field of one JSON record: derived from the abort
state and the first fault domain, no stored string is needed.
*/
void write_record_failure_reason(std::ostream& out, const SimulationResult& r)
{
    out << "    \"failure_reason\": ";
    if (r.mission_aborted) {
        out << "\"Safe mode engage : ";
        write_json_escaped(out, fault_domain_name(r.first_fault_domain));
        out << "\"";
    }
    else {
        out << "\"\"";
    }
    out << ",\n";
}

}

/*
Writes the timing, metric and verdict fields of one JSON record, closing the
object.
*/
void write_record_metrics(std::ostream& out, const ScenarioRecord& record, bool last)
{
    const SimulationResult& r = record.result;

    out << "    \"fault_injected_time\": ";
    write_seconds(out, r.fault_injected_time);
    out << ",\n";
    out << "    \"detection_time\": ";
    write_seconds(out, r.detection_time);
    out << ",\n";
    out << "    \"recovery_time\": ";
    write_seconds(out, r.recovery_time);
    out << ",\n";
    out << "    \"detection_latency\": ";
    write_seconds(out, r.detection_latency);
    out << ",\n";
    out << "    \"response_latency\": ";
    write_seconds(out, r.response_latency);
    out << ",\n";
    out << "    \"max_position_error\": ";
    write_metric(out, r.max_position_error_m);
    out << ",\n";
    out << "    \"max_altitude_error\": ";
    write_metric(out, r.max_altitude_error_m);
    out << ",\n";
    out << "    \"final_altitude\": ";
    write_metric(out, r.final_altitude_m);
    out << ",\n";
    write_record_failure_reason(out, r);
    out << "    \"passed\": " << (r.passed ? "true" : "false") << "\n";
    out << "  }" << (last ? "" : ",") << "\n";
}

}