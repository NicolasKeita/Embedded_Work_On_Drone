/*
Filename: Src/SIL/Validation/FlightControlValidation-CsvWriter.cpp
Description: Campaign CSV stream writer : fixed header row and per-run row orchestration of Monte-Carlo outcomes.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

namespace sim::sil::validation {

/*
Writes the fixed CSV header row. Semicolons are used as delimiters so that
locale-dependent comma handling never splits a numeric field.
*/
static void write_csv_header(std::ostream& out)
{
    out << "run_id;scenario_seed;failure_mode;failure_mode_name;fault_target;fault_target_name;"
           "fault_profile;fault_start_time_s;fault_duration_s;fault_loss_probability;"
           "sensor_corruption;actuator_efficiency;wind_x_mps;wind_y_mps;"
           "ambient_pressure_kpa;ambient_temperature_k;sensor_noise_altitude_m;"
           "sensor_noise_position_m;comms_loss_probability;comms_transport_latency_s;"
           "comms_link_up;initial_altitude_m;initial_x_m;initial_y_m;target_altitude_m;"
           "simulation_duration_s;time_step_s;mission_success;final_state;final_state_name;"
           "final_health;final_health_name;final_safety_mode;final_safety_mode_name;"
           "degraded_reached;compensated_reached;safe_mode_reached;fault_detected;"
           "fault_injected_time_s;detection_time_s;safety_response_time_s;"
           "detection_latency_s;response_latency_s;recovery_attempted;recovery_successful;"
           "recovery_time_s;max_position_error_m;mean_position_error_m;"
           "max_altitude_error_m;mean_altitude_error_m;final_x_m;final_y_m;final_altitude_m;"
           "max_pitch_rad;max_roll_rad;comms_sent;comms_delivered;comms_dropped;"
           "comms_timeouts;supervision_triggered;supervision_trigger_time_s;test_verdict;"
           "verdict;failure_reason;failure_reason_id;failure_reason_name;"
           "position_error_m;altitude_error_m\n";
}

/*
Writes the full CSV stream of the campaign: the fixed header row followed by
one row per stored run. The header is emitted by write_csv_header and every
row by write_csv_row so that this function stays a thin loop.
*/
void ResultCollector::write_csv(std::ostream& out) const
{
    write_csv_header(out);
    for (const SimulationResult& result : results_) {
        write_csv_row(out, result);
    }
}

}
