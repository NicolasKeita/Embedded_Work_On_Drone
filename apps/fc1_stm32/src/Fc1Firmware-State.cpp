/*
Filename: apps/fc1_stm32/src/Fc1Firmware-State.cpp
Description: Shared mutable state of the FC1 firmware units: inter-FC link bookkeeping, the
status LED instance and its supervision-driven update.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>

module Fc1Firmware;

import std;

import InterFcLink;
import StatusLed;

namespace fc1 {

Fc1InterFc inter_fc{};
FlightCore::Status::StatusLed status_led{};

/* Advances the status LED from the current inter-FC supervision state. */
void update_status_led() noexcept
{
    const std::int64_t now_ms = k_uptime_get();
    const bool remote_missing = inter_fc.last_remote_status_ms < 0
        ? now_ms >= 2000 : now_ms - inter_fc.last_remote_status_ms >= 500;
    const bool led_fault = remote_missing || inter_fc.heartbeat_suppressed != 0
        || inter_fc.remote_state == static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Degraded)
        || inter_fc.remote_state == static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Safe)
        || inter_fc.remote_detection != static_cast<std::uint8_t>(FlightCore::InterFc::DetectionCode::None);
    static_cast<void>(status_led.update(now_ms, led_fault));
}

}
