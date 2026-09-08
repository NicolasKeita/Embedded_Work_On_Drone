/*
Filename: Src/Safety/HealthMonitor-Core.cpp
Description: Health evaluation logic of the FC2 monitor.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

import Aircraft;
import CommsBus;
import Telemetry;

namespace sim::safety {

HealthMonitor::HealthMonitor(HealthMonitorConfig config) : config_{config} {}

std::size_t HealthMonitor::detection_index(DetectionEvent event)
{
    return static_cast<std::size_t>(event);
}

HealthState HealthMonitor::state() const noexcept
{
    return state_;
}

void HealthMonitor::raise(DetectionEvent event, std::float64_t time)
{
    DetectionFlag& item = flags_[detection_index(event)];

    if (!item.raised) {
        item.raised = true;
        item.raised_time = time;
    }
}

HealthState HealthMonitor::compute_state(const std::array<DetectionFlag, kDetectionEventCount>& flags)
{
    const bool critical = flags[detection_index(DetectionEvent::FC1_HEARTBEAT_TIMEOUT)].raised
        || flags[detection_index(DetectionEvent::COMMUNICATION_TIMEOUT)].raised;

    if (critical) {
        return HealthState::SAFE;
    }
    const bool degraded = flags[detection_index(DetectionEvent::SENSOR_VALIDATION_FAILED)].raised
        || flags[detection_index(DetectionEvent::ACTUATOR_MISMATCH)].raised;
    if (degraded) {
        return HealthState::DEGRADED;
    }
    return HealthState::HEALTHY;
}

}
