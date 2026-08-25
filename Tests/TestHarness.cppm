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
    double dt{0.01};
    std::size_t log_interval_steps{100};
};

class TestHarness {
public:
    explicit TestHarness(HarnessConfig config = {}) : config_(config) {}

    // Assertions.
    void check(bool condition, std::string_view label);
    [[nodiscard]] int failure_count() const noexcept { return failures_; }
    [[nodiscard]] bool passed() const noexcept { return failures_ == 0; }

    // Logging & execution.
    void log_header() const;
    void log_step(const Aircraft& aircraft) const;
    void log_step(const Aircraft& aircraft, double time_seconds) const;
    void run(Aircraft& aircraft, double duration_seconds);

    // Contextual helpers / scenarios.
    void take_off(Aircraft& aircraft, double target_rpm);
    void reset();

private:
    HarnessConfig config_;
    int failures_{0};
    double current_time_{0.0};
    std::size_t step_count_{0};
};

} // namespace sim::test
