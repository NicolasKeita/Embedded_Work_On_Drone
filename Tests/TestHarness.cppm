/*
Filename: Tests/TestHarness.cppm
Description: Encapsulated test harness for aircraft simulation scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module TestHarness;

import std;

import Aircraft;

export namespace sim::test {

struct HarnessConfig {
    std::float64_t dt{0.01};
    std::size_t    log_interval_steps{100};
};

class TestHarness {
public:
    explicit TestHarness(HarnessConfig config = {}) : config_(config) {}

    // Assertions.
    void check(bool condition, std::string_view label);
    [[nodiscard]] std::uint32_t failure_count() const noexcept { return failures_; }
    [[nodiscard]] bool passed() const noexcept { return failures_ == 0; }

    // Logging & execution.
    void log_header() const;
    void log_step(const Aircraft& aircraft) const;
    void log_step(const Aircraft& aircraft, std::float64_t time_seconds) const;
    void run(Aircraft& aircraft, std::float64_t duration_seconds);

    // Contextual helpers / scenarios.
    void take_off(Aircraft& aircraft, std::float64_t target_rpm);
    void reset();

private:
    HarnessConfig  config_;
    std::uint32_t  failures_{0};
    std::float64_t current_time_{0.0};
    std::size_t    step_count_{0};
};

}
