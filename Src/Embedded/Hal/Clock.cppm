/*
Filename: Src/Embedded/Hal/Clock.cppm
Description: HAL abstraction over the time source. The PC mock (SimulatedClock)
    is advanced deterministically by the lockstep driver; the STM32 HIL
    implementation is backed by a hardware timer / DWT cycle counter.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Clock;

import std;

export namespace FlightCore::HAL
{

/*
    Monotonic time source in microseconds. nowUs() reports the current instant;
    sleepUs() blocks (real target) or advances the virtual clock (PC lockstep).
*/
class IClock
{
public:
    IClock() = default;
    virtual ~IClock() = default;

    IClock(const IClock&)            = delete;
    IClock& operator=(const IClock&) = delete;

    [[nodiscard]] virtual std::uint64_t nowUs() const noexcept = 0;

    virtual void sleepUs(std::uint64_t us) noexcept = 0;
};

}
