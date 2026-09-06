/*
Filename: Src/Embedded/Hil/HilClock.cppm
Description: Wall-clock abstraction for the HIL runner. IWallClock exposes nowUs()
and absolute-point sleepUntilUs() so the runner can schedule on a drift-free
absolute deadline. MonotonicClock is backed by std::chrono::steady_clock (the real
wall-clock source for pacing, transport latency and deadline monitoring); FastClock
advances instantly for deterministic, accelerated unit tests. Simulation time is
kept out of this module entirely: only wall clock lives here.
Exports:
    class IWallClock,
    class MonotonicClock,
    class FastClock

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilClock;

import std;

export namespace sim::hil {

class IWallClock {
public:
    IWallClock() = default;
    virtual ~IWallClock() = default;

    IWallClock(const IWallClock&)            = delete;
    IWallClock& operator=(const IWallClock&) = delete;

    [[nodiscard]] virtual std::uint64_t nowUs() const noexcept = 0;

    /*
    Blocks (or, for FastClock, no-ops) until the wall clock reaches abs_us, used
    for absolute scheduling. If abs_us is already in the past the call returns at
    once so overruns are carried by the runner rather than compounding drift.
    */
    virtual void sleepUntilUs(std::uint64_t abs_us) noexcept = 0;
};

/*
Real wall-clock backed by the monotonic steady clock. The HIL runner schedules
each cycle on an absolute steady-clock deadline so a step that ran long shortens
(or skips) the next sleep instead of accumulating drift via relative sleeps.
*/
class MonotonicClock final : public IWallClock {
public:
    MonotonicClock() = default;

    [[nodiscard]] std::uint64_t nowUs() const noexcept override;

    void sleepUntilUs(std::uint64_t abs_us) noexcept override;

private:
    static std::chrono::steady_clock::time_point to_time_point(std::uint64_t us) noexcept;
    static std::chrono::steady_clock::time_point epoch() noexcept;
};

/*
Instant wall-clock for deterministic unit tests: nowUs() advances only when the
test drives it and sleepUntilUs() never blocks, so the closed loop runs as fast as
the CPU allows while the deadline logic stays exercisable through setNowUs().
*/
class FastClock final : public IWallClock {
public:
    FastClock() = default;

    void setNowUs(std::uint64_t us) noexcept;

    [[nodiscard]] std::uint64_t nowUs() const noexcept override;

    void sleepUntilUs(std::uint64_t abs_us) noexcept override;

private:
    std::uint64_t now_us_ = 0;
};

inline void FastClock::setNowUs(std::uint64_t us) noexcept
{
    now_us_ = us;
}

inline std::uint64_t FastClock::nowUs() const noexcept
{
    return now_us_;
}

inline void FastClock::sleepUntilUs(std::uint64_t abs_us) noexcept
{
    if (abs_us > now_us_) {
        now_us_ = abs_us;
    }
}

}
