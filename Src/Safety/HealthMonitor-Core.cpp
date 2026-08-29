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

std::size_t HealthMonitor::domain_index(FaultDomain domain)
{
    return static_cast<std::size_t>(domain);
}

HealthState HealthMonitor::state() const noexcept
{
    return state_;
}

void HealthMonitor::raise(FaultDomain domain, double time)
{
    FaultFlag& item = flags_[domain_index(domain)];
    if (!item.raised) {
        item.raised = true;
        item.raised_time = time;
    }
}

HealthState HealthMonitor::compute_state(const std::array<FaultFlag, 4>& flags)
{
    const bool critical = flags[domain_index(FaultDomain::FC1Heartbeat)].raised
        || flags[domain_index(FaultDomain::Communication)].raised;
    if (critical) {
        return HealthState::SAFE;
    }
    const bool degraded = flags[domain_index(FaultDomain::Sensor)].raised
        || flags[domain_index(FaultDomain::Actuator)].raised;
    if (degraded) {
        return HealthState::DEGRADED;
    }
    return HealthState::HEALTHY;
}

}
