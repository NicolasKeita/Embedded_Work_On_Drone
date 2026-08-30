/*
Filename: Src/SIL/Reporting/SilReporting-Export.cpp
Description: Streaming CSV writer for SIL simulation results export.

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
Writes the full CSV payload of the SIL results into the given stream.
*/
void write_csv_payload(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "name;fault_type;start_time;duration;mission_success;mission_aborted;"
           "final_state;final_health;final_safety_mode;fault_detected;fault_injected_time;"
           "detection_time;recovery_time;detection_latency;response_latency;"
           "max_position_error;max_altitude_error;final_altitude;failure_reason;passed\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        write_csv_escaped(out, record.name);
        out << ';' << fault_type_name(record.scenario.fault_type) << ';';
        write_seconds(out, record.scenario.start_time);
        out << ';';
        write_seconds(out, record.scenario.duration);
        out << ';' << (r.mission_success ? 1 : 0) << ';' << (r.mission_aborted ? 1 : 0) << ';'
            << sim::control::mission_state_name(r.final_state) << ';'
            << health_state_name(r.final_health) << ';' << safety_mode_name(r.final_safety_mode)
            << ';' << (r.fault_detected ? 1 : 0) << ';';
        write_seconds(out, r.fault_injected_time);
        out << ';';
        write_seconds(out, r.detection_time);
        out << ';';
        write_seconds(out, r.recovery_time);
        out << ';';
        write_seconds(out, r.detection_latency);
        out << ';';
        write_seconds(out, r.response_latency);
        out << ';';
        write_metric(out, r.max_position_error_m);
        out << ';';
        write_metric(out, r.max_altitude_error_m);
        out << ';';
        write_metric(out, r.final_altitude_m);
        out << ';';
        if (r.mission_aborted) {
            write_csv_escaped(out, fault_domain_name(r.first_fault_domain));
        }
        out << ';' << (r.passed ? 1 : 0) << '\n';
    }
}

}
