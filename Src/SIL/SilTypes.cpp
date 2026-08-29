/*
Filename: Src/SIL/SilTypes.cpp
Description: Fault type naming helper for the SIL data structures.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTypes;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import Telemetry;

namespace sim::sil {

/*
Synthetic assertion summary: mission success, safety reaction consistent with
the fault (SAFE_MODE for critical faults, COMPENSATED for degrading faults) and
bounded detection latency, except for the actuator fault whose detection depends
on the flight dynamics.
*/
bool SimulationResult::compute_verdict(bool fault_expected) const
{
    if (fault_expected != fault_detected) {
        return false;
    }
    if (fault_detected && first_fault_domain != sim::safety::FaultDomain::FC1Heartbeat
        && first_fault_domain != sim::safety::FaultDomain::Actuator && detection_latency > 0.5) {
        return false;
    }
    if (mission_aborted) {
        return final_safety_mode == sim::safety::SafetyMode::SAFE_MODE;
    }
    if (final_safety_mode == sim::safety::SafetyMode::COMPENSATED) {
        return final_health == sim::safety::HealthState::DEGRADED;
    }
    return mission_success && final_health == sim::safety::HealthState::HEALTHY
        && final_safety_mode == sim::safety::SafetyMode::NORMAL;
}

std::string_view fault_type_name(FaultType type)
{
    switch (type) {
    case FaultType::None:
        return "NOMINAL";
    case FaultType::FC1Failure:
        return "FC1_FAILURE";
    case FaultType::CommunicationLoss:
        return "COMMUNICATION_LOSS";
    case FaultType::CommunicationLossRate:
        return "COMMUNICATION_LOSS_RATE";
    case FaultType::SensorFault:
        return "SENSOR_FAULT";
    case FaultType::ActuatorDegradation:
        return "ACTUATOR_DEGRADATION";
    }
    return "UNKNOWN";
}

}
