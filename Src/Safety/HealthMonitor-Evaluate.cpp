/*
Filename: Src/Safety/HealthMonitor-Evaluate.cpp
Description: FC2 health evaluation : comms/heartbeat, sensor and actuator flag updates.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

import Aircraft;
import CommsBus;
import Telemetry;

namespace sim::safety {

/*
Communication link loss and FC1 heartbeat timeout raise latched flags: these are
physical events that only a reset can clear.
*/
void HealthMonitor::update_comms_flags(double current_time, const sim::sil::CommsBus& comms)
{
    if (!comms.link_up()) {
        raise(FaultDomain::Communication, current_time);
    }
    else {
        const double last_received = comms.last_received_time();
        if (last_received >= 0.0
            && current_time - last_received > config_.heartbeat_timeout_s) {
            raise(FaultDomain::FC1Heartbeat, current_time);
        }
    }
}

/*
Sensor faults are re-evaluated continuously: the flag clears as soon as the
telemetry validates again.
*/
void HealthMonitor::update_sensor_flags(double                           current_time,
                                        const sim::sil::SensorTelemetry& telemetry)
{
    if (!validate(telemetry, config_.sensor_limits).all_valid()) {
        raise(FaultDomain::Sensor, current_time);
    }
    else {
        flags_[domain_index(FaultDomain::Sensor)].raised = false;
    }
}

/*
The commanded/actual RPM mismatch must be sustained during
actuator_mismatch_hold_s before the flag is raised; otherwise it clears.
*/
void HealthMonitor::update_actuator_flags(double                           current_time,
                                          const sim::sil::SensorTelemetry& telemetry,
                                          double                           commanded_rpm)
{
    const double mismatch = std::abs(commanded_rpm - telemetry.actual_rpm);
    const bool mismatching =
        commanded_rpm > 0.0 && mismatch > config_.actuator_mismatch_rpm;
    if (mismatching && mismatch_since_ < 0.0) {
        mismatch_since_ = current_time;
    }
    if (!mismatching) {
        mismatch_since_ = -1.0;
    }
    const bool sustained = mismatching
        && current_time - mismatch_since_ >= config_.actuator_mismatch_hold_s;
    if (sustained) {
        raise(FaultDomain::Actuator, current_time);
    }
    else {
        flags_[domain_index(FaultDomain::Actuator)].raised = false;
    }
}

/*
Full health evaluation: updates the domain flags, recomputes the overall state
and returns the report snapshot.
*/
HealthReport HealthMonitor::evaluate(double                           current_time,
                                     const sim::sil::CommsBus&        comms,
                                     const sim::sil::SensorTelemetry& telemetry,
                                     double                           commanded_rpm)
{
    update_comms_flags(current_time, comms);
    update_sensor_flags(current_time, telemetry);
    update_actuator_flags(current_time, telemetry, commanded_rpm);
    state_ = compute_state(flags_);
    HealthReport report;
    report.state = state_;
    report.flags = flags_;
    return report;
}

}
