/*
Filename: embedded/sim/sim_clock.cppm
Description: PC mock of the time source for deterministic lockstep execution
(hil_architecture.md 6.2 / hil_protocol.md 6.1). The virtual clock is advanced
explicitly by advanceUs(); sleepUs() advances it too instead of blocking on a
wall clock, so every run is bit-reproducible.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.sim.clock;

import std;

import flight.hal.clock;

export namespace FlightCore::Sim
{

class SimulatedClock final : public FlightCore::HAL::IClock
{
public:
    SimulatedClock() = default;

    explicit SimulatedClock(std::uint64_t start_us) noexcept;

    /* Advances the virtual clock by us (drives one lockstep step). */
    void advanceUs(std::uint64_t us) noexcept;

    void setUs(std::uint64_t us) noexcept;

    [[nodiscard]] std::uint64_t nowUs() const noexcept override;

    /* Advances the virtual clock deterministically (no wall-clock blocking). */
    void sleepUs(std::uint64_t us) noexcept override;

private:
    std::uint64_t now_{0};
};

inline SimulatedClock::SimulatedClock(std::uint64_t start_us) noexcept
    : now_{start_us}
{
}

inline void SimulatedClock::advanceUs(std::uint64_t us) noexcept
{
    now_ += us;
}

inline void SimulatedClock::setUs(std::uint64_t us) noexcept
{
    now_ = us;
}

inline std::uint64_t SimulatedClock::nowUs() const noexcept
{
    return now_;
}

inline void SimulatedClock::sleepUs(std::uint64_t us) noexcept
{
    now_ += us;
}

}
