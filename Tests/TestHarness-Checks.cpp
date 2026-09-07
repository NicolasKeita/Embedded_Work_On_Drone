/*
Filename: Tests/TestHarness-Checks.cpp
Description: Context tags, assertions and state reset of the shared validation harness.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TestHarness;

import std;

namespace sim::test {

void TestHarness::set_context(std::string_view tag)
{
    context_ = std::string{tag};
}

void TestHarness::begin_scenario(std::string_view id, std::string_view description)
{
    context_ = std::string{id};
    std::cout << "\n=== [" << id << "][" << run_target_tag(config_.target) << "] "
              << description << " ===" << std::endl;
}

/*
Checks a condition and explicitly logs the success or failure, pairing the active
scenario/sub-suite tag with the execution target ([tag][target]); the failure is
counted in the runner instance state (failures_).
*/
void TestHarness::check(bool condition, std::string_view label)
{
    std::cout << "  [" << (condition ? "PASS" : "FAIL") << "] ";
    if (!context_.empty()) {
        std::cout << '[' << context_ << "][" << run_target_tag(config_.target) << "] ";
    }
    std::cout << label << std::endl;
    if (!condition) {
        ++failures_;
    }
}

void TestHarness::reset()
{
    failures_ = 0;
    current_time_ = 0.0;
    step_count_ = 0;
    overshoot_ = 0.0;
    settling_time_ = 0.0;
    steady_state_error_ = 0.0;
    max_acceleration_ = 0.0;
}

void TestHarness::record_metrics(std::float64_t overshoot, std::float64_t settling_time,
                                 std::float64_t steady_state_error, std::float64_t max_acceleration)
{
    overshoot_ = overshoot;
    settling_time_ = settling_time;
    steady_state_error_ = steady_state_error;
    max_acceleration_ = max_acceleration;
}

}
