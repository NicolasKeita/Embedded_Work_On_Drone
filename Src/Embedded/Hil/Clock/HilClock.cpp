/*
Filename: Src/Embedded/Hil/Clock/HilClock.cpp
Description: MonotonicClock implementation : steady-clock nowUs() and absolute-point
sleepUntilUs() backed by std::this_thread::sleep_for.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilClock;

import std;

namespace {
    const std::chrono::steady_clock::time_point kEpoch = std::chrono::steady_clock::now();
}

namespace sim::hil {

std::chrono::steady_clock::time_point MonotonicClock::epoch() noexcept
{
    return kEpoch;
}

std::chrono::steady_clock::time_point MonotonicClock::to_time_point(std::uint64_t us) noexcept
{
    return epoch() + std::chrono::microseconds(us);
}

std::uint64_t MonotonicClock::nowUs() const noexcept
{
    const auto delta = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - epoch());

    return static_cast<std::uint64_t>(delta.count());
}

void MonotonicClock::sleepUntilUs(std::uint64_t abs_us) noexcept
{
    const auto target = to_time_point(abs_us);
    const auto now = std::chrono::steady_clock::now();

    if (target <= now) {
        return;
    }
    std::this_thread::sleep_for(target - now);
}

}
