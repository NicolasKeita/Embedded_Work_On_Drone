/*
Filename: apps/fc2_stm32/src/Fc2Firmware-Convert.cpp
Description: FC2 type conversions between the inter-FC vocabulary and the shared health,
safety and telemetry cores.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>

module Fc2Firmware;

import std;

import HealthMonitor;
import InterFcLink;
import SafetyManager;
import Telemetry;

namespace fc2 {

namespace {
    /* Restarts the health and safety cores at their configured timeouts. */
    sim::safety::HealthMonitor make_monitor()
    {
        return sim::safety::HealthMonitor{sim::safety::HealthMonitorConfig{
            .heartbeat_timeout_s = kHeartbeatTimeoutSeconds,
            .initial_heartbeat_timeout_s = kInitialHeartbeatTimeoutSeconds,
        }};
    }
}

/* Maps the safety manager state to the transport-independent FC-to-FC status. */
FlightCore::InterFc::NodeState to_node_state(sim::safety::SafetyMode mode) noexcept
{
    switch (mode) {
    case sim::safety::SafetyMode::NORMAL:
        return FlightCore::InterFc::NodeState::Healthy;
    case sim::safety::SafetyMode::COMPENSATED:
        return FlightCore::InterFc::NodeState::Degraded;
    case sim::safety::SafetyMode::SAFE_MODE:
        return FlightCore::InterFc::NodeState::Safe;
    }
    return FlightCore::InterFc::NodeState::Unknown;
}

/* Maps the first active FC2 detection to the inter-FC diagnostic vocabulary. */
FlightCore::InterFc::DetectionCode to_detection_code(const sim::safety::HealthReport& report) noexcept
{
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        if (!report.flags[index].raised) {
            continue;
        }
        switch (static_cast<sim::safety::DetectionEvent>(index)) {
        case sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT:
            return FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout;
        case sim::safety::DetectionEvent::COMMUNICATION_TIMEOUT:
            return FlightCore::InterFc::DetectionCode::CommunicationTimeout;
        case sim::safety::DetectionEvent::SENSOR_VALIDATION_FAILED:
            return FlightCore::InterFc::DetectionCode::SensorValidationFailed;
        case sim::safety::DetectionEvent::ACTUATOR_MISMATCH:
            return FlightCore::InterFc::DetectionCode::ActuatorMismatch;
        }
    }
    return FlightCore::InterFc::DetectionCode::None;
}

/* Converts a HIL sensor packet into the telemetry consumed by the FC2 core. */
sim::sil::SensorTelemetry to_telemetry(const FlightCore::HAL::SensorData& sensor) noexcept
{
    return sim::sil::SensorTelemetry{
        .x = sensor.position_x_m,
        .y = sensor.position_y_m,
        .z = sensor.altitude_baro_m,
        .vx = sensor.velocity_x_ms,
        .vy = sensor.velocity_y_ms,
        .vz = sensor.velocity_z_ms,
        .pitch = sensor.pitch_rad,
        .roll = sensor.roll_rad,
        .actual_rpm = sensor.wing_rpm_meas,
    };
}

/* Restarts the health and safety supervision when a new run resets the session. */
void reset_supervision(Fc2Health& health) noexcept
{
    health.monitor = make_monitor();
    health.safety = sim::safety::SafetyManager{};
    health.reset_requested = false;
    health.telemetry = sim::sil::SensorTelemetry{};
    health.commanded_rpm = 0.0;
    sim::safety::rearm_link_supervision(
        health.supervision, static_cast<std::float64_t>(k_uptime_get()) / 1000.0);
}

}
