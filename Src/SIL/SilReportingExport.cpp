/*
Filename: Src/SIL/SilReportingExport.cpp
Description: JSON and CSV builders for SIL simulation results export.

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

std::string build_json(const std::vector<ScenarioRecord>& records)
{
    std::ostringstream out;
    out << "[\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        const ScenarioRecord& record = records[index];
        const SimulationResult& r = record.result;
        out << "  {\n";
        out << "    \"name\": \"" << json_escape(record.name) << "\",\n";
        out << "    \"fault_type\": \"" << fault_type_name(record.scenario.fault_type)
            << "\",\n";
        out << "    \"start_time\": " << format_seconds(record.scenario.start_time) << ",\n";
        out << "    \"duration\": " << format_seconds(record.scenario.duration) << ",\n";
        out << "    \"mission_success\": " << (r.mission_success ? "true" : "false") << ",\n";
        out << "    \"mission_aborted\": " << (r.mission_aborted ? "true" : "false") << ",\n";
        out << "    \"final_state\": \""
            << json_escape(sim::control::mission_state_name(r.final_state)) << "\",\n";
        out << "    \"final_health\": \"" << health_state_name(r.final_health) << "\",\n";
        out << "    \"final_safety_mode\": \"" << safety_mode_name(r.final_safety_mode)
            << "\",\n";
        out << "    \"fault_detected\": " << (r.fault_detected ? "true" : "false") << ",\n";
        out << "    \"fault_injected_time\": " << format_seconds(r.fault_injected_time)
            << ",\n";
        out << "    \"detection_time\": " << format_seconds(r.detection_time) << ",\n";
        out << "    \"recovery_time\": " << format_seconds(r.recovery_time) << ",\n";
        out << "    \"detection_latency\": " << format_seconds(r.detection_latency) << ",\n";
        out << "    \"response_latency\": " << format_seconds(r.response_latency) << ",\n";
        out << "    \"max_position_error\": " << format_metric(r.max_position_error_m) << ",\n";
        out << "    \"max_altitude_error\": " << format_metric(r.max_altitude_error_m) << ",\n";
        out << "    \"final_altitude\": " << format_metric(r.final_altitude_m) << ",\n";
        out << "    \"failure_reason\": \"" << json_escape(r.failure_reason) << "\",\n";
        out << "    \"passed\": " << (r.passed ? "true" : "false") << "\n";
        out << "  }" << (index + 1 < records.size() ? "," : "") << "\n";
    }
    out << "]\n";
    return out.str();
}

std::string build_csv(const std::vector<ScenarioRecord>& records)
{
    std::ostringstream out;
    out << "name;fault_type;start_time;duration;mission_success;mission_aborted;"
           "final_state;final_health;final_safety_mode;fault_detected;fault_injected_time;"
           "detection_time;recovery_time;detection_latency;response_latency;"
           "max_position_error;max_altitude_error;final_altitude;failure_reason;passed\n";
    for (const ScenarioRecord& record : records) {
        const SimulationResult& r = record.result;
        out << csv_escape(record.name) << ';' << fault_type_name(record.scenario.fault_type)
            << ';' << record.scenario.start_time << ';' << record.scenario.duration << ';'
            << (r.mission_success ? 1 : 0) << ';' << (r.mission_aborted ? 1 : 0) << ';'
            << sim::control::mission_state_name(r.final_state) << ';'
            << health_state_name(r.final_health) << ';'
            << safety_mode_name(r.final_safety_mode) << ';' << (r.fault_detected ? 1 : 0)
            << ';' << r.fault_injected_time << ';' << r.detection_time << ';'
            << r.recovery_time << ';' << r.detection_latency << ';' << r.response_latency
            << ';' << r.max_position_error_m << ';' << r.max_altitude_error_m << ';'
            << r.final_altitude_m << ';' << csv_escape(r.failure_reason) << ';'
            << (r.passed ? 1 : 0) << '\n';
    }
    return out.str();
}

}

std::string json_payload(const std::vector<ScenarioRecord>& records)
{
    return build_json(records);
}

std::string csv_payload(const std::vector<ScenarioRecord>& records)
{
    return build_csv(records);
}

}
