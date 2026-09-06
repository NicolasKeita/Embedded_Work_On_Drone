/*
Filename: Src/Embedded/Hil/HilTiming.cppm
Description: Per-step timing record and deadline statistics for the real-time HIL
loop. HilStepTiming captures every host-wall-clock instant of one closed-loop cycle
(scheduled, actual start, sensor send, FC receive/send when available, actuator
receive, aircraft update, step completion) and derives the step execution time, the
wall-clock round trip, the deadline margin, the lateness and the deadline-missed
flag against the configured control period. HilTimingStats accumulates min/mean/max
step and round-trip times, the number of deadline misses, the executed step count and
the maximum lateness across a whole run.
Exports:
    struct HilStepTiming,
    struct HilTimingStats

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilTiming;

import std;

export namespace sim::hil {

struct HilStepTiming {
    std::uint64_t scheduled_us = 0;
    std::uint64_t actual_start_us = 0;
    std::uint64_t sensor_send_us = 0;
    std::int64_t  fc_receive_us = -1;
    std::int64_t  fc_send_us = -1;
    std::uint64_t actuator_receive_us = 0;
    std::uint64_t aircraft_update_us = 0;
    std::uint64_t step_completion_us = 0;

    [[nodiscard]] std::int64_t step_execution_us() const noexcept;
    [[nodiscard]] std::int64_t round_trip_us() const noexcept;
    [[nodiscard]] std::int64_t deadline_margin_us(std::uint64_t period_us) const noexcept;
    [[nodiscard]] std::int64_t lateness_us(std::uint64_t period_us) const noexcept;
    [[nodiscard]] bool deadline_missed(std::uint64_t period_us) const noexcept;
};

inline std::int64_t HilStepTiming::step_execution_us() const noexcept
{
    return static_cast<std::int64_t>(step_completion_us) - static_cast<std::int64_t>(actual_start_us);
}

inline std::int64_t HilStepTiming::round_trip_us() const noexcept
{
    return static_cast<std::int64_t>(actuator_receive_us) - static_cast<std::int64_t>(sensor_send_us);
}

inline std::int64_t HilStepTiming::deadline_margin_us(std::uint64_t period_us) const noexcept
{
    const std::int64_t budget = static_cast<std::int64_t>(scheduled_us) + static_cast<std::int64_t>(period_us);
    return budget - static_cast<std::int64_t>(step_completion_us);
}

inline std::int64_t HilStepTiming::lateness_us(std::uint64_t period_us) const noexcept
{
    return -deadline_margin_us(period_us);
}

inline bool HilStepTiming::deadline_missed(std::uint64_t period_us) const noexcept
{
    return step_completion_us > (scheduled_us + period_us);
}

struct HilTimingStats {
    std::uint64_t steps_executed = 0;
    std::uint64_t deadline_misses = 0;
    std::int64_t  min_step_us = -1;
    std::int64_t  max_step_us = -1;
    std::int64_t  min_round_trip_us = -1;
    std::int64_t  max_round_trip_us = -1;
    std::int64_t  max_lateness_us = -1;
    std::float64_t mean_step_us = 0.0;
    std::float64_t mean_round_trip_us = 0.0;

    void record(const HilStepTiming& timing, std::uint64_t period_us);
};

}
