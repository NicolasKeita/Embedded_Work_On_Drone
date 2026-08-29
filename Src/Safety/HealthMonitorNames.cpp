/*
Filename: Src/Safety/HealthMonitorNames.cpp
Description: Readable naming helpers for health states and fault domains.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

namespace sim::safety {

std::string_view fault_domain_name(FaultDomain domain)
{
    switch (domain) {
    case FaultDomain::FC1Heartbeat:
        return "FC1_HEARTBEAT_TIMEOUT";
    case FaultDomain::Communication:
        return "COMMUNICATION_LOST";
    case FaultDomain::Sensor:
        return "SENSOR_INVALID";
    case FaultDomain::Actuator:
        return "ACTUATOR_MISMATCH";
    }
    return "UNKNOWN";
}

std::string_view health_state_name(HealthState state)
{
    switch (state) {
    case HealthState::HEALTHY:
        return "HEALTHY";
    case HealthState::DEGRADED:
        return "DEGRADED";
    case HealthState::SAFE:
        return "SAFE";
    case HealthState::FAILED:
        return "FAILED";
    }
    return "UNKNOWN";
}

}
