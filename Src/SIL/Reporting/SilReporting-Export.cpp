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

namespace {

/*
Writes the scenario identity and mission fields of one CSV record.
*/
void write_csv_record_mission(std::ostream& out, const ScenarioRecord& record)
{
    const SimulationResult& r = record.result;

    write_csv_escaped(out, record.name);
    out << ';' << fault_type_name(record.scenario.fault_type) << ';';
    write_seconds(out, record.scenario.start_time);
    out << ';';
    write_seconds(out, record.scenario.duration);
    out << ';' << (r.mission_success ? 1 : 0) << ';' << sim::control::mission_state_name(r.final_state) << ';';
    write_seconds(out, r.mission_duration_s);
    out << ';' << health_state_name(r.final_health) << ';' << safety_mode_name(r.final_safety_mode)
        << ';' << (r.fault_detected ? 1 : 0) << ';';
}

/*
Writes the fault timing and recovery fields of one CSV record.
*/
void write_csv_record_fault(std::ostream& out, const SimulationResult& r)
{
    write_seconds(out, r.fault_injected_time);
    out << ';';
    write_seconds(out, r.detection_time);
    out << ';';
    write_seconds(out, r.safety_response_time);
    out << ';' << (r.recovery_attempted ? 1 : 0) << ';' << (r.recovery_successful ? 1 : 0) << ';';
    write_seconds(out, r.recovery_time);
    out << ';';
    write_seconds(out, r.detection_latency);
    out << ';';
    write_seconds(out, r.response_latency);
    out << ';';
}

/*
Writes the aircraft metrics fields of one CSV record.
*/
void write_csv_record_aircraft(std::ostream& out, const SimulationResult& r)
{
    write_metric(out, r.max_position_error_m);
    out << ';';
    write_metric(out, r.mean_position_error_m);
    out << ';';
    write_metric(out, r.max_altitude_error_m);
    out << ';';
    write_metric(out, r.mean_altitude_error_m);
    out << ';';
    write_metric(out, r.final_x_m);
    out << ';';
    write_metric(out, r.final_y_m);
    out << ';';
    write_metric(out, r.final_altitude_m);
    out << ';';
    write_metric(out, r.max_pitch_rad);
    out << ';';
    write_metric(out, r.max_roll_rad);
    out << ';';
}

/*
Writes the watchdog, communication and verdict fields of one CSV record.
*/
void write_csv_record_comms(std::ostream& out, const SimulationResult& r)
{
    out << (r.watchdog_triggered ? 1 : 0) << ';' << r.comms.sent << ';' << r.comms.delivered << ';'
        << r.comms.dropped << ';' << r.comms.timeouts << ';';
    write_seconds(out, r.comms.latency_min_s);
    out << ';';
    write_seconds(out, r.comms.latency_mean_s);
    out << ';';
    write_seconds(out, r.comms.latency_max_s);
    out << ';' << (r.test_verdict ? 1 : 0) << '\n';
}

}

/*
Writes the full CSV payload of the SIL results into the given stream.
*/
void write_csv_payload(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "name;fault_type;start_time;duration;mission_success;final_state;mission_duration;"
           "final_health;final_safety_mode;fault_detected;fault_injected_time;detection_time;"
           "safety_response_time;recovery_attempted;recovery_successful;recovery_time;"
           "detection_latency;response_latency;max_position_error;mean_position_error;"
           "max_altitude_error;mean_altitude_error;final_x;final_y;final_altitude;max_pitch_rad;"
           "max_roll_rad;watchdog_triggered;comms_sent;comms_delivered;comms_dropped;comms_timeouts;"
           "comms_latency_min;comms_latency_mean;comms_latency_max;test_verdict\n";
    for (const ScenarioRecord& record : records) {
        write_csv_record_mission(out, record);
        write_csv_record_fault(out, record.result);
        write_csv_record_aircraft(out, record.result);
        write_csv_record_comms(out, record.result);
    }
}
}
