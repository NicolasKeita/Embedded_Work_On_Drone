/*
Filename: Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cpp
Description: Safety-behaviour verdict of a HIL run (fault detection matching, detection
latency budget and terminal mission/safety/health state consistency).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerTypes;

import std;

import FlightControllerTypes;
import HealthMonitor;
import SafetyManager;

namespace sim::hil {

bool HilResult::compute_verdict(bool fault_expected) const noexcept
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

}
