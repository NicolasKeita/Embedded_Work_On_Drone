/*
Filename: Src/SIL/SilTypes.cppm
Description: SIL data structures : fault scenarios, simulated environment and simulation results.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilTypes;

import std;

import FlightController;
import Telemetry;
import HealthMonitor;
import SafetyManager;

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
    double loss_probability = 0.0;
    SensorCorruptionMode corruption = SensorCorruptionMode::None;
    double corrupted_altitude_m = 99999.0;
    double efficiency = 1.0;
};

struct FaultScenario {
    double start_time = 0.0;
    double duration = 0.0;
    FaultType fault_type = FaultType::None;
    FaultParameters parameters{};
};

// Simulated environment that fault injectors are allowed to alter.
struct SimulationState {
    bool fc1_alive = true;
    bool comms_link_up = true;
    double comms_loss_probability = 0.0;
    double actuator_efficiency = 1.0;
    SensorCorruptionMode sensor_corruption = SensorCorruptionMode::None;
    double corrupted_altitude_m = 0.0;
};

struct SimulationResult {
    bool mission_success = false;
    bool mission_aborted = false;
    sim::control::MissionState final_state = sim::control::MissionState::TAKEOFF;
    sim::safety::HealthState final_health = sim::safety::HealthState::HEALTHY;
    sim::safety::SafetyMode final_safety_mode = sim::safety::SafetyMode::NORMAL;
    bool fault_detected = false;
    bool degraded_reached = false;
    bool compensated_reached = false;
    bool safe_mode_reached = false;
    sim::safety::FaultDomain first_fault_domain = sim::safety::FaultDomain::FC1Heartbeat;
    std::string failure_reason;

    double max_position_error_m = 0.0;
    double max_altitude_error_m = 0.0;
    double final_altitude_m = 0.0;

    double fault_injected_time = -1.0;
    double detection_time = -1.0;
    double recovery_time = -1.0;
    double detection_latency = -1.0;
    double response_latency = -1.0;

    bool passed = false;

    [[nodiscard]] bool compute_verdict(bool fault_expected) const;
};

// Human-readable name of a fault type for reports.
[[nodiscard]] std::string_view fault_type_name(FaultType type);

}
