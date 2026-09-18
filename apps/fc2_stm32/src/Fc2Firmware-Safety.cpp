/*
Filename: apps/fc2_stm32/src/Fc2Firmware-Safety.cpp
Description: FC2 health evaluation and periodic status publication: applies the safety
decision to the monitored link and reports FC2 health on the inter-FC transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

module Fc2Firmware;

import std;

import HealthMonitor;
import InterFcLink;
import SafetyManager;

namespace fc2 {

FlightCore::InterFc::NodeState evaluate_fc1_health(Fc2Health& health,
                                                   FlightCore::InterFc::DetectionCode& detection) noexcept
{
    const std::float64_t now = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
    const sim::safety::SafetyMode previous_mode = health.safety.mode();
    const sim::safety::HealthReport report =
        health.monitor.evaluate(now, health.supervision, health.telemetry, health.commanded_rpm);
    static_cast<void>(health.safety.update(now, report));
    const FlightCore::InterFc::NodeState node_state = to_node_state(health.safety.mode());
    inter_fc.node_state = static_cast<std::uint8_t>(node_state);
    detection = to_detection_code(report);
    if (health.safety.mode() != previous_mode) {
        printk("[HEALTH] role=FC2 health=%s safety=%s heartbeat_age_ms=%d\n",
               sim::safety::health_state_name(report.state).data(),
               sim::safety::safety_mode_name(health.safety.mode()).data(),
               health.supervision.last_heartbeat_time >= 0.0
                   ? static_cast<int>((now - health.supervision.last_heartbeat_time) * 1000.0)
                   : -1);
    }
    return node_state;
}

void publish_fc2_status(FlightCore::InterFc::IInterFcTransport& transport,
                        FlightCore::InterFc::NodeState node_state,
                        FlightCore::InterFc::DetectionCode detection,
                        std::int64_t& last_status_ms) noexcept
{
    const std::int64_t now_ms = k_uptime_get();
    if (now_ms - last_status_ms < kStatusPeriodMs) {
        return;
    }
    const FlightCore::InterFc::Message status{
        .kind = FlightCore::InterFc::MessageKind::Status,
        .sequence = inter_fc.last_sequence,
        .state = node_state,
        .detection = detection,
    };
    static_cast<void>(transport.send(status));
    last_status_ms = now_ms;
}

}
