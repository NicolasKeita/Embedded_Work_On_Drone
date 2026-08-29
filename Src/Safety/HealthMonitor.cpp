/*
Filename: Src/Safety/HealthMonitor.cpp
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

/*
FC1/communication faults are latched (physical events); sensor/actuator faults
are re-evaluated continuously (the condition may disappear).
*/
HealthReport HealthMonitor::evaluate(double                           current_time,
                                     const sim::sil::CommsBus&        comms,
                                     const sim::sil::SensorTelemetry& telemetry,
                                     double                           commanded_rpm)
{
    if (!comms.link_up()) {
        raise(FaultDomain::Communication, current_time);
    }
    else {
        const double last_received = comms.last_received_time();
        if (last_received >= 0.0 && current_time - last_received > config_.heartbeat_timeout_s) {
            raise(FaultDomain::FC1Heartbeat, current_time);
        }
    }
    if (!validate(telemetry, config_.sensor_limits).all_valid()) {
        raise(FaultDomain::Sensor, current_time);
    }
    else {
        flags_[domain_index(FaultDomain::Sensor)].raised = false;
    }
    const double mismatch = std::abs(commanded_rpm - telemetry.actual_rpm);
    const bool mismatching =
        commanded_rpm > 0.0 && mismatch > config_.actuator_mismatch_rpm;
    if (mismatching && mismatch_since_ < 0.0) {
        mismatch_since_ = current_time;
    }
    if (!mismatching) {
        mismatch_since_ = -1.0;
    }
    const bool sustained =
        mismatching && current_time - mismatch_since_ >= config_.actuator_mismatch_hold_s;
    if (sustained) {
        raise(FaultDomain::Actuator, current_time);
    }
    else {
        flags_[domain_index(FaultDomain::Actuator)].raised = false;
    }
    state_ = compute_state(flags_);
    HealthReport report;
    report.state = state_;
    report.flags = flags_;
    return report;
}

}
