/*
Filename: Src/SIL/Core/SilTypes.cpp
Description: Simulation result helpers : synthetic verdict and textual verdict reason.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTypes;

import std;

import Aircraft;
import CommsBus;
import FlightController;
import HealthMonitor;
import SafetyManager;
import Telemetry;

namespace sim::sil {

/*
Synthetic assertion summary: mission success, safety reaction consistent with
the failure mode (SAFE_MODE for critical faults, COMPENSATED for degrading
faults) and bounded detection latency, except for the actuator degradation
whose detection depends on the flight dynamics.
*/
bool SimulationResult::compute_verdict(bool fault_expected) const
{
    if (fault_expected != fault_detected) {
        return false;
    }
    if (fault_detected && first_detection_event != sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT
        && first_detection_event != sim::safety::DetectionEvent::ACTUATOR_MISMATCH && detection_latency > 0.5) {
        return false;
    }
    if (final_state == sim::control::MissionState::ABORTED) {
        return final_safety_mode == sim::safety::SafetyMode::SAFE_MODE;
    }
    if (final_safety_mode == sim::safety::SafetyMode::COMPENSATED) {
        return final_health == sim::safety::HealthState::DEGRADED;
    }
    return mission_success && final_health == sim::safety::HealthState::HEALTHY
        && final_safety_mode == sim::safety::SafetyMode::NORMAL;
}

/*
Human-readable verdict reason: explains the PASS/FAIL decision independently
from the mission outcome (an aborted mission can still be a PASS).
*/
std::string_view verdict_reason(const SimulationResult& result)
{
    if (!result.test_verdict) {
        return "Comportement non conforme aux attendus du scenario";
    }
    if (result.final_state == sim::control::MissionState::ABORTED) {
        return "Defaillance detectee et SAFE_MODE engage dans les limites requises";
    }
    if (result.final_safety_mode == sim::safety::SafetyMode::COMPENSATED) {
        return "Defaillance degradee compensee, mission poursuivie";
    }
    return "Mission nominale completee sans comportement anormal";
}

}
