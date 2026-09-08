/*
Filename: Src/SIL/Core/SilTypes.cppm
Description: SIL data structures : simulated environment and simulation results (scenario types come from SilFaultScenario).
Exports:
    struct SimulationState,
    struct SimulationResult,
    verdict_reason()

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

export import SilFaultScenario;

export namespace sim::sil {

/*
Simulated environment that failure-mode injectors are allowed to alter: the
physical representation levers of each failure mode (FC1 liveness, link state,
packet loss probability, actuator efficiency, sensor corruption).
*/
struct SimulationState {
    bool                 fc1_alive = true;
    bool                 comms_link_up = true;
    std::float64_t       comms_loss_probability = 0.0;
    std::float64_t       actuator_efficiency = 1.0;
    SensorCorruptionMode sensor_corruption = SensorCorruptionMode::None;
    std::float64_t       corrupted_altitude_m = 0.0;
};

/*
Full structured outcome of one SIL run. Mission, fault chain, aircraft,
communication, heartbeat supervision and verdict groups are kept independent
so that automated validation (and future Monte Carlo campaigns) can consume
every field individually.
*/
struct SimulationResult {
    // Mission (COMPLETE means success; ABORTED/FAILED are documented terminal states).
    bool                       mission_success = false;
    sim::control::MissionState final_state = sim::control::MissionState::TAKEOFF;
    std::float64_t             mission_duration_s = -1.0;

    // Safety.
    sim::safety::HealthState    final_health = sim::safety::HealthState::HEALTHY;
    sim::safety::SafetyMode     final_safety_mode = sim::safety::SafetyMode::NORMAL;
    bool                        degraded_reached = false;
    bool                        compensated_reached = false;
    bool                        safe_mode_reached = false;
    sim::safety::DetectionEvent first_detection_event = sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT;

    // Fault chain: injection -> detection -> response -> recovery.
    FailureMode    failure_mode = FailureMode::NONE;
    bool           fault_detected = false;
    std::float64_t fault_injected_time = -1.0;
    std::float64_t detection_time = -1.0;
    std::float64_t safety_response_time = -1.0;
    std::float64_t detection_latency = -1.0;
    std::float64_t response_latency = -1.0;
    bool           recovery_attempted = false;
    bool           recovery_successful = false;
    std::float64_t recovery_time = -1.0;

    // Aircraft.
    std::float64_t max_position_error_m = 0.0;
    std::float64_t mean_position_error_m = 0.0;
    std::float64_t max_altitude_error_m = 0.0;
    std::float64_t mean_altitude_error_m = 0.0;
    std::float64_t final_x_m = 0.0;
    std::float64_t final_y_m = 0.0;
    std::float64_t final_altitude_m = 0.0;
    std::float64_t max_pitch_rad = 0.0;
    std::float64_t max_roll_rad = 0.0;

    // Communication (FC1 <-> FC2).
    CommsStats comms{};

    // Heartbeat supervision (FC1 liveness / link monitoring on FC2).
    bool           supervision_triggered = false;
    std::float64_t supervision_trigger_time = -1.0;

    // Test verdict: did the system behave as the scenario requires.
    bool test_verdict = false;

    [[nodiscard]] bool compute_verdict(bool fault_expected) const;
};

// Human-readable reason of the test verdict for reports.
[[nodiscard]] std::string_view verdict_reason(const SimulationResult& result);

}
