/*
Filename: Src/SIL/Validation/FlightControlValidation-CsvRows.cpp
Description: Per-run CSV column writers : scenario-side and SIL-result-side field formatting of Monte-Carlo outcomes.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilTypes;
import FlightController;
import HealthMonitor;
import SafetyManager;

namespace sim::sil::validation {

/*
Writes the scenario-side columns of one CSV row: run id, seed, fault family,
environment, sensors, comms and initial conditions. Enum fields are exported
both as their numeric id and their human-readable name.
*/
static void write_scenario_fields(std::ostream& out, const SimulationResult& result)
{
    const Scenario& s = result.scenario;
    out << result.run_id << ';' << result.scenario_seed << ';'
        << static_cast<std::uint32_t>(s.fault_type) << ';' << fault_type_name(s.fault_type) << ';'
        << static_cast<std::uint32_t>(s.fault_target) << ';' << fault_target_name(s.fault_target) << ';'
        << static_cast<std::uint32_t>(s.fault_profile) << ';' << s.fault_start_time_s << ';'
        << s.fault_duration_s << ';' << s.fault_loss_probability << ';'
        << static_cast<std::uint32_t>(s.sensor_corruption) << ';' << s.actuator_efficiency << ';'
        << s.wind_x_mps << ';' << s.wind_y_mps << ';' << s.ambient_pressure_kpa << ';'
        << s.ambient_temperature_k << ';' << s.sensor_noise_altitude_m << ';'
        << s.sensor_noise_position_m << ';' << s.comms_loss_probability << ';'
        << s.comms_transport_latency_s << ';' << (s.comms_link_up ? 1 : 0) << ';'
        << s.initial_altitude_m << ';' << s.initial_x_m << ';' << s.initial_y_m << ';'
        << s.target_altitude_m << ';' << s.simulation_duration_s << ';' << s.time_step_s << ';';
}

/*
Writes the SIL-result-side columns of one CSV row: mission, safety, fault
chain, aircraft, comms, watchdog and verdict fields. The Monte-Carlo verdict
and failure reason close the row.
*/
static void write_sil_result_fields(std::ostream& out, const SimulationResult& result)
{
    const sim::sil::SimulationResult& r = result.sil_result;
    out << (r.mission_success ? 1 : 0) << ';' << static_cast<std::uint32_t>(r.final_state) << ';'
        << sim::control::mission_state_name(r.final_state) << ';'
        << static_cast<std::uint32_t>(r.final_health) << ';'
        << sim::safety::health_state_name(r.final_health) << ';'
        << static_cast<std::uint32_t>(r.final_safety_mode) << ';'
        << sim::safety::safety_mode_name(r.final_safety_mode) << ';'
        << (r.degraded_reached ? 1 : 0) << ';' << (r.compensated_reached ? 1 : 0) << ';'
        << (r.safe_mode_reached ? 1 : 0) << ';' << (r.fault_detected ? 1 : 0) << ';'
        << r.fault_injected_time << ';' << r.detection_time << ';' << r.safety_response_time << ';'
        << r.detection_latency << ';' << r.response_latency << ';'
        << (r.recovery_attempted ? 1 : 0) << ';' << (r.recovery_successful ? 1 : 0) << ';'
        << r.recovery_time << ';' << r.max_position_error_m << ';' << r.mean_position_error_m << ';'
        << r.max_altitude_error_m << ';' << r.mean_altitude_error_m << ';' << r.final_x_m << ';'
        << r.final_y_m << ';' << r.final_altitude_m << ';' << r.max_pitch_rad << ';' << r.max_roll_rad << ';'
        << r.comms.sent << ';' << r.comms.delivered << ';' << r.comms.dropped << ';'
        << r.comms.timeouts << ';' << (r.watchdog_triggered ? 1 : 0) << ';' << r.watchdog_trigger_time << ';'
        << (r.test_verdict ? 1 : 0) << ';' << (result.verdict ? 1 : 0) << ';'
        << static_cast<std::uint32_t>(result.failure_reason) << ';'
        << failure_reason_id(result.failure_reason) << ';' << failure_reason_name(result.failure_reason) << ';'
        << result.position_error_m << ';' << result.altitude_error_m << '\n';
}

/*
Writes one full CSV row for a run by concatenating the scenario-side and
SIL-result-side column groups. Declared in the module interface so that the
stream writer can call it from its own implementation file.
*/
void write_csv_row(std::ostream& out, const SimulationResult& result)
{
    write_scenario_fields(out, result);
    write_sil_result_fields(out, result);
}

}
