/*
Filename: Src/Embedded/Status/StatusLed.cpp
Description: Zephyr user LED patterns driven by application progress.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

module StatusLed;

import std;

namespace FlightCore::Status {

namespace {
const gpio_dt_spec user_led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
}

/* Configures the user LED off without making indicator failure fatal to control. */
std::expected<void, std::int32_t> StatusLed::initialize() noexcept
{
    if (!gpio_is_ready_dt(&user_led)) {
        return std::unexpected(-ENODEV);
    }
    const std::int32_t result = gpio_pin_configure_dt(&user_led, GPIO_OUTPUT_INACTIVE);
    if (result < 0) {
        return std::unexpected(result);
    }
    initialized_ = true;
    return {};
}

/* Records completed work, keeping the active indication for half a second. */
void StatusLed::mark_activity(std::int64_t now_ms) noexcept
{
    last_activity_ms_ = now_ms;
}

/* Shows 1 Hz idle, 4 Hz activity, or three 100 ms flashes followed by a pause. */
std::expected<void, std::int32_t> StatusLed::update(std::int64_t now_ms, bool fault) noexcept
{
    if (!initialized_) {
        return std::unexpected(-ENODEV);
    }
    const bool active = last_activity_ms_ >= 0 && now_ms - last_activity_ms_ < 500;
    const std::uint8_t next_mode = fault ? 2 : (active ? 1 : 0);
    if (next_mode != mode_) {
        mode_ = next_mode;
        pattern_started_ms_ = now_ms;
    }
    const std::int64_t elapsed_ms = now_ms - pattern_started_ms_;
    const std::int64_t phase_ms = elapsed_ms % 2000;
    const bool illuminated = mode_ == 2
        ? (phase_ms < 600 && phase_ms % 200 < 100)
        : elapsed_ms % (active ? 250 : 1000) < (active ? 125 : 500);
    if (illuminated == illuminated_) {
        return {};
    }
    const std::int32_t result = gpio_pin_set_dt(&user_led, illuminated ? 1 : 0);
    if (result < 0) {
        return std::unexpected(result);
    }
    illuminated_ = illuminated;
    return {};
}

}
