/*
Filename: Src/Embedded/Hil/Config/HilConfig.cpp
Description: Configuration helpers for the HIL runner : deadline policy naming and the
configured control step count.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilConfig;

import std;

namespace sim::hil {

/* Evaluates the active interval and smooth periodic gusts without random state. */
std::float64_t wind_factor(const HilConfig& config, std::float64_t time_s) noexcept
{
    if (time_s < config.wind_start_s || time_s >= config.wind_end_s) {
        return 0.0;
    }
    if (config.wind_gust_period_s <= 0.0) {
        return 1.0;
    }
    const std::float64_t phase = (time_s - config.wind_start_s) / config.wind_gust_period_s;
    return 0.5 - 0.5 * std::cos(2.0 * std::numbers::pi * phase);
}

/* Returns the deadline policy label. */
std::string_view deadline_policy_name(DeadlinePolicy policy) noexcept
{
    switch (policy) {
    case DeadlinePolicy::Warn:
        return "WARN";
    case DeadlinePolicy::Fail:
        return "FAIL";
    case DeadlinePolicy::Abort:
        return "ABORT";
    }
    return "WARN";
}

std::uint64_t hil_step_count(const HilConfig& config) noexcept
{
    if (config.dt_s <= 0.0) {
        return 0;
    }
    const std::float64_t steps = config.duration_s / config.dt_s;
    return static_cast<std::uint64_t>(std::llround(steps));
}

}
