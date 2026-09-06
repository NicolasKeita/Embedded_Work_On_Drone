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

// Fully resets the runner state: failure counter, simulated time and steps.
void TestHarness::reset()
{
    failures_ = 0;
    current_time_ = 0.0;
    step_count_ = 0;
}

}
