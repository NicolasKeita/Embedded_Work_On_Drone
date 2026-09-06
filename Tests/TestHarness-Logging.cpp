/*
Filename: Tests/TestHarness-Logging.cpp
Description: State logging and run loop of the shared validation harness (periodic
telemetry table, take-off phase helper).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TestHarness;

import std;

import Aircraft;

namespace
{
    constexpr std::float64_t kTakeOffDurationSeconds = 3.0;
}

namespace sim::test {

void TestHarness::log_header() const
{
    std::cout << "     t(s)";
    std::cout << "   " << std::setw(11) << "x(m)" << std::setw(11) << "y(m)"
              << std::setw(11) << "z(m)";
    std::cout << "   " << std::setw(11) << "vx(m/s)" << std::setw(11) << "vy(m/s)"
              << std::setw(11) << "vz(m/s)";
    std::cout << "   " << std::setw(11) << "pitch(deg)" << std::setw(11) << "roll(deg)";
    std::cout << "   " << std::setw(11) << "rpm" << std::endl;
}

void TestHarness::log_step(const Aircraft& aircraft) const
{
    log_step(aircraft, current_time_);
}

/*
Variant with an explicit timestamp: used by the autonomous scenarios which
advance their control loop manually, without going through run().
*/
void TestHarness::log_step(const Aircraft& aircraft, std::float64_t time_seconds) const
{
    const AircraftState& s = aircraft.state();

    std::cout << std::fixed << std::setw(9) << std::setprecision(2) << time_seconds;
    std::cout << "   " << std::setprecision(3) << std::setw(11) << s.x
              << std::setw(11) << s.y
              << std::setw(11) << s.z;
    std::cout << "   " << std::setw(11) << s.vx
              << std::setw(11) << s.vy
              << std::setw(11) << s.vz;
    std::cout << "   " << std::setprecision(2) << std::setw(11) << s.pitch * 180.0 / std::numbers::pi
              << std::setw(11) << s.roll * 180.0 / std::numbers::pi;
    std::cout << "   " << std::setprecision(0) << std::setw(11) << s.actual_rpm
              << std::defaultfloat << std::endl;
}

/*
Advances the simulation by durationSeconds on the given aircraft, tracking the
simulated time and step count in the instance state, and periodically logging
the state at the rate defined by HarnessConfig::log_interval_steps. A final log
is emitted only if the last step was not already logged by the periodicity
(otherwise the last table row would be duplicated).
*/
void TestHarness::run(Aircraft& aircraft, std::float64_t duration_seconds)
{
    const std::uint32_t steps = static_cast<std::uint32_t>(duration_seconds / config_.dt + 0.5);
    bool                last_step_logged = false;

    for (std::uint32_t i = 0; i < steps; ++i) {
        aircraft.update(config_.dt);
        ++step_count_;
        current_time_ += config_.dt;

        if (config_.log_interval_steps > 0 && step_count_ % config_.log_interval_steps == 0) {
            log_step(aircraft);
            last_step_logged = true;
        }
    }

    if (!last_step_logged) {
        log_step(aircraft);
    }
}

/*
Phase common to the aerial scenarios: fast climb to gain altitude. The simulated
time is tracked internally by the runner (current_time_), so the phase no longer
needs to return its end instant.
*/
void TestHarness::take_off(Aircraft& aircraft, std::float64_t target_rpm)
{
    aircraft.set_command({1.3 * target_rpm, 0.0, 0.0});
    run(aircraft, kTakeOffDurationSeconds);
}

}
