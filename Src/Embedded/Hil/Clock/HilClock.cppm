/*
Filename: Src/Embedded/Hil/Clock/HilClock.cppm
Description: Monotonic wall clock for the HIL runner. MonotonicClock exposes nowUs()
and absolute-point sleepUntilUs() so the runner can schedule on a drift-free
absolute deadline. It is backed by std::chrono::steady_clock and is the
wall-clock source for pacing, transport latency and deadline monitoring. Simulation
time is kept out of this module entirely: only wall clock lives here.
Exports:
    class MonotonicClock

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilClock;

import std;

export namespace sim::hil {

/*
Real wall-clock backed by the monotonic steady clock. The HIL runner schedules
each cycle on an absolute steady-clock deadline so a step that ran long shortens
(or skips) the next sleep instead of accumulating drift via relative sleeps.
*/
class MonotonicClock final {
public:
    MonotonicClock() = default;

    [[nodiscard]] std::uint64_t nowUs() const noexcept;

    /*
    Blocks until the wall clock reaches abs_us. If it is already in the past,
    returns immediately so overruns are carried rather than compounding drift.
    */
    void sleepUntilUs(std::uint64_t abs_us) noexcept;

private:
    static std::chrono::steady_clock::time_point to_time_point(std::uint64_t us) noexcept;
    static std::chrono::steady_clock::time_point epoch() noexcept;
};

}
