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
