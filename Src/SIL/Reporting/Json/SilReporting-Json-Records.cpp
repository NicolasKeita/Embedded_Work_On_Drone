/*
Filename: Src/SIL/Reporting/Json/SilReporting-Json-Records.cpp
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

void write_record_failure_reason(std::ostream& out, const SimulationResult& r);

/* Writes the identity, mission and fault fields of one JSON record. */
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
    out << "    \"final_state\": \"";
    write_json_escaped(out, sim::control::mission_state_name(r.final_state));
    out << "\",\n";
    out << "    \"mission_duration\": ";
    write_seconds(out, r.mission_duration_s);
    out << ",\n";
    out << "    \"final_health\": \"" << health_state_name(r.final_health) << "\",\n";
    out << "    \"final_safety_mode\": \"" << safety_mode_name(r.final_safety_mode) << "\",\n";
    out << "    \"fault_detected\": " << (r.fault_detected ? "true" : "false") << ",\n";
}

namespace {

/* Writes one seconds-valued JSON field with a trailing comma. */
void write_seconds_field(std::ostream& out, std::string_view name, std::float64_t value, bool comma = true)
{
    out << "    \"" << name << "\": ";
    write_seconds(out, value);
    out << (comma ? ",\n" : "\n");
}

/* Writes one metric-valued JSON field with a trailing comma. */
void write_metric_field(std::ostream& out, std::string_view name, std::float64_t value)
{
    out << "    \"" << name << "\": ";
    write_metric(out, value);
    out << ",\n";
}

}

/* Writes timing, metric, comms, watchdog and verdict fields, closing the object. */
void write_record_metrics(std::ostream& out, const ScenarioRecord& record, bool last)
{
    const SimulationResult& r = record.result;

    write_seconds_field(out, "fault_injected_time", r.fault_injected_time);
    write_seconds_field(out, "detection_time", r.detection_time);
    write_seconds_field(out, "safety_response_time", r.safety_response_time);
    write_seconds_field(out, "recovery_time", r.recovery_time);
    write_seconds_field(out, "detection_latency", r.detection_latency);
    write_seconds_field(out, "response_latency", r.response_latency);
    out << "    \"recovery_attempted\": " << (r.recovery_attempted ? "true" : "false") << ",\n";
    out << "    \"recovery_successful\": " << (r.recovery_successful ? "true" : "false") << ",\n";
    write_metric_field(out, "max_position_error", r.max_position_error_m);
    write_metric_field(out, "mean_position_error", r.mean_position_error_m);
    write_metric_field(out, "max_altitude_error", r.max_altitude_error_m);
    write_metric_field(out, "mean_altitude_error", r.mean_altitude_error_m);
    write_metric_field(out, "final_altitude", r.final_altitude_m);
    write_metric_field(out, "max_pitch_rad", r.max_pitch_rad);
    write_metric_field(out, "max_roll_rad", r.max_roll_rad);
    out << "    \"watchdog_triggered\": " << (r.watchdog_triggered ? "true" : "false") << ",\n";
    write_seconds_field(out, "watchdog_trigger_time", r.watchdog_trigger_time);
    out << "    \"telemetry_samples\": " << record.telemetry.size() << ",\n";
    out << "    \"ground_truth_samples\": " << record.ground_truth.size() << ",\n";
    out << "    \"comms\": {\"sent\": " << r.comms.sent << ", \"delivered\": " << r.comms.delivered
        << ", \"dropped\": " << r.comms.dropped << ", \"duplicated\": " << r.comms.duplicated
        << ", \"reordered\": " << r.comms.reordered << ", \"timeouts\": " << r.comms.timeouts
        << ", \"latency_min\": ";
    write_seconds(out, r.comms.latency_min_s);
    out << ", \"latency_mean\": ";
    write_seconds(out, r.comms.latency_mean_s);
    out << ", \"latency_max\": ";
    write_seconds(out, r.comms.latency_max_s);
    out << "},\n";
    write_record_failure_reason(out, r);
    out << "    \"test_verdict\": " << (r.test_verdict ? "true" : "false") << "\n";
    out << "  }" << (last ? "" : ",") << "\n";
}

}
