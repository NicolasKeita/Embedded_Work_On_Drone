/*
Filename: Src/Embedded/Hil/Clock/HilTiming.cpp
Description: HilTimingStats accumulation : step/round-trip extrema and means, deadline
miss counting and maximum lateness.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTiming;

import std;

namespace sim::hil {

std::int64_t HilStepTiming::deadline_margin_us(std::uint64_t period_us) const noexcept
{
    return static_cast<std::int64_t>(scheduled_us + period_us) - static_cast<std::int64_t>(step_completion_us);
}

void HilTimingStats::record(const HilStepTiming& timing, std::uint64_t period_us)
{
    const std::int64_t step = timing.step_execution_us();
    const std::int64_t rtt = timing.round_trip_us();

    min_step_us = (min_step_us < 0) ? step : std::min(min_step_us, step);
    max_step_us = (max_step_us < 0) ? step : std::max(max_step_us, step);
    min_round_trip_us = (min_round_trip_us < 0) ? rtt : std::min(min_round_trip_us, rtt);
    max_round_trip_us = (max_round_trip_us < 0) ? rtt : std::max(max_round_trip_us, rtt);

    mean_step_us = (mean_step_us * static_cast<std::float64_t>(steps_executed)
                    + static_cast<std::float64_t>(step))
                 / static_cast<std::float64_t>(steps_executed + 1);
    mean_round_trip_us = (mean_round_trip_us * static_cast<std::float64_t>(steps_executed)
                          + static_cast<std::float64_t>(rtt))
                       / static_cast<std::float64_t>(steps_executed + 1);

    if (timing.deadline_missed(period_us)) {
        ++deadline_misses;
        max_lateness_us = std::max(max_lateness_us, timing.lateness_us(period_us));
    }
    ++steps_executed;
}

}
