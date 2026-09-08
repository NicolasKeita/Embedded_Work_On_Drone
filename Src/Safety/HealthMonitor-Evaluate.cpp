/*
Filename: Src/Safety/HealthMonitor-Evaluate.cpp
Description: FC2 health evaluation : comms/heartbeat supervision, sensor and actuator detection flag updates.

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
Communication link loss and FC1 heartbeat timeout raise latched detection
flags: these are physical events that only a reset can clear.
*/
void HealthMonitor::update_comms_flags(std::float64_t current_time, const sim::sil::CommsBus& comms)
{
    if (!comms.link_up()) {
        raise(DetectionEvent::COMMUNICATION_TIMEOUT, current_time);
    }
    else {
        const std::float64_t last_received = comms.last_received_time();
        if (last_received >= 0.0 && current_time - last_received > config_.heartbeat_timeout_s) {
            raise(DetectionEvent::FC1_HEARTBEAT_TIMEOUT, current_time);
        }
    }
}

/*
Sensor validation is re-evaluated continuously: the detection flag clears as
soon as the telemetry validates again.
*/
void HealthMonitor::update_sensor_flags(std::float64_t                   current_time,
                                        const sim::sil::SensorTelemetry& telemetry)
{
    if (!validate(telemetry, config_.sensor_limits).all_valid()) {
        raise(DetectionEvent::SENSOR_VALIDATION_FAILED, current_time);
    }
    else {
        flags_[detection_index(DetectionEvent::SENSOR_VALIDATION_FAILED)].raised = false;
    }
}

/*
The commanded/actual RPM mismatch must be sustained during
actuator_mismatch_hold_s before the detection flag is raised; otherwise it
clears.
*/
void HealthMonitor::update_actuator_flags(std::float64_t                   current_time,
                                          const sim::sil::SensorTelemetry& telemetry,
                                          std::float64_t                   commanded_rpm)
{
    const std::float64_t mismatch = std::abs(commanded_rpm - telemetry.actual_rpm);
    const bool           mismatching = commanded_rpm > 0.0 && mismatch > config_.actuator_mismatch_rpm;

    if (mismatching && mismatch_since_ < 0.0) {
        mismatch_since_ = current_time;
    }
    if (!mismatching) {
        mismatch_since_ = -1.0;
    }
    const bool sustained = mismatching && current_time - mismatch_since_ >= config_.actuator_mismatch_hold_s;
    if (sustained) {
        raise(DetectionEvent::ACTUATOR_MISMATCH, current_time);
    }
    else {
        flags_[detection_index(DetectionEvent::ACTUATOR_MISMATCH)].raised = false;
    }
}

/*
Full health evaluation: updates the detection flags, recomputes the overall
state and returns the report snapshot.
*/
HealthReport HealthMonitor::evaluate(std::float64_t                   current_time,
                                     const sim::sil::CommsBus&        comms,
                                     const sim::sil::SensorTelemetry& telemetry,
                                     std::float64_t                   commanded_rpm)
{
    update_comms_flags(current_time, comms);
    update_sensor_flags(current_time, telemetry);
    update_actuator_flags(current_time, telemetry, commanded_rpm);
    state_ = compute_state(flags_);
    return HealthReport{.state = state_, .flags = flags_};
}

}
