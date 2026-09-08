/*
Filename: Src/Safety/HealthMonitor-Names.cpp
Description: Readable naming helpers for health states and detection events.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

namespace sim::safety {

std::string_view detection_event_name(DetectionEvent event)
{
    switch (event) {
    case DetectionEvent::FC1_HEARTBEAT_TIMEOUT:
        return "FC1_HEARTBEAT_TIMEOUT";
    case DetectionEvent::COMMUNICATION_TIMEOUT:
        return "COMMUNICATION_TIMEOUT";
    case DetectionEvent::SENSOR_VALIDATION_FAILED:
        return "SENSOR_VALIDATION_FAILED";
    case DetectionEvent::ACTUATOR_MISMATCH:
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
    }
    return "UNKNOWN";
}

}
