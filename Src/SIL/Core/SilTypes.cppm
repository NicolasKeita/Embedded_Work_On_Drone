/*
Filename: Src/SIL/Core/SilTypes.cppm
Description: SIL data structures : fault scenarios, simulated environment and simulation results.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilTypes;

import std;

import Aircraft;
import CommsBus;
import FlightController;
import HealthMonitor;
import SafetyManager;
import Telemetry;

export namespace sim::sil {

enum class FaultType {
    None,
    FC1Failure,
    CommunicationLoss,
    CommunicationLossRate,
    SensorFault,
    ActuatorDegradation
};

struct FaultParameters {
    double               loss_probability = 0.0;
    SensorCorruptionMode corruption = SensorCorruptionMode::None;
    double               corrupted_altitude_m = 99999.0;
    double               efficiency = 1.0;
};

struct FaultScenario {
    double          start_time = 0.0;
    double          duration = 0.0;
    FaultType       fault_type = FaultType::None;
    FaultParameters parameters{};
};

// Simulated environment that fault injectors are allowed to alter.
struct SimulationState {
    bool                 fc1_alive = true;
    bool                 comms_link_up = true;
    double               comms_loss_probability = 0.0;
    double               actuator_efficiency = 1.0;
    SensorCorruptionMode sensor_corruption = SensorCorruptionMode::None;
    double               corrupted_altitude_m = 0.0;
};

/*
Full structured outcome of one SIL run. Mission, fault, aircraft, communication,
watchdog and verdict groups are kept independent so that automated validation
(and future Monte Carlo campaigns) can consume every field individually.
*/
struct SimulationResult {
    // Mission (COMPLETE means success; ABORTED/FAILED are documented terminal states).
    bool                       mission_success = false;
    sim::control::MissionState final_state = sim::control::MissionState::TAKEOFF;
    double                     mission_duration_s = -1.0;

    // Safety.
    sim::safety::HealthState final_health = sim::safety::HealthState::HEALTHY;
    sim::safety::SafetyMode  final_safety_mode = sim::safety::SafetyMode::NORMAL;
    bool                     degraded_reached = false;
    bool                     compensated_reached = false;
    bool                     safe_mode_reached = false;
    sim::safety::FaultDomain first_fault_domain = sim::safety::FaultDomain::FC1Heartbeat;

    // Fault chain: injection -> detection -> response -> recovery.
    FaultType fault_type = FaultType::None;
    bool      fault_detected = false;
    double    fault_injected_time = -1.0;
    double    detection_time = -1.0;
    double    safety_response_time = -1.0;
    double    detection_latency = -1.0;
    double    response_latency = -1.0;
    bool      recovery_attempted = false;
    bool      recovery_successful = false;
    double    recovery_time = -1.0;

    // Aircraft.
    double max_position_error_m = 0.0;
    double mean_position_error_m = 0.0;
    double max_altitude_error_m = 0.0;
    double mean_altitude_error_m = 0.0;
    double final_x_m = 0.0;
    double final_y_m = 0.0;
    double final_altitude_m = 0.0;
    double max_pitch_rad = 0.0;
    double max_roll_rad = 0.0;

    // Communication (FC1 <-> FC2).
    CommsStats comms{};

    // Watchdog (heartbeat/comms supervision on FC2).
    bool   watchdog_triggered = false;
    double watchdog_trigger_time = -1.0;

    // Test verdict: did the system behave as the scenario requires.
    bool test_verdict = false;

    [[nodiscard]] bool compute_verdict(bool fault_expected) const;
};

// Human-readable name of a fault type for reports.
[[nodiscard]] std::string_view fault_type_name(FaultType type);

// Human-readable reason of the test verdict for reports.
[[nodiscard]] std::string_view verdict_reason(const SimulationResult& result);

}
