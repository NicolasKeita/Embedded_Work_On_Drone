/*
Filename: Tests/Scenarios/Scenarios.cppm
Description: Deterministic validation scenarios (A to F) and the launchable scenario registry.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Scenarios;

import std;

import TestHarness;

export namespace sim::test {

using ScenarioFn = void (*)(TestHarness&, std::float64_t);

struct ScenarioEntry {
    char             key;
    std::string_view description;
    ScenarioFn       run;
};

class ScenarioCatalog {
public:
    [[nodiscard]] static std::span<const ScenarioEntry> all() noexcept;
    [[nodiscard]] static const ScenarioEntry* find(char argument) noexcept;
    static void print_usage(std::string_view executableName);

private:
    static const std::array<ScenarioEntry, 10> scenarios_;
};

}

export namespace sim::test::scenarios {

// Scenario A: the vehicle is at rest, it must stay on the ground.
void rest(TestHarness& runner, std::float64_t hover_rpm);

// Scenario B: RPM above hover, vertical climb.
void climb(TestHarness& runner, std::float64_t hover_rpm);

// Scenario C: climb then throttle down, return to the ground.
void descent(TestHarness& runner, std::float64_t hover_rpm);

// Scenario D: positive servo mean command (+10 degrees) -> pitch > 0.
void move_x(TestHarness& runner, std::float64_t hover_rpm);

// Scenario E: opposed servos (+12 / -12 degrees), pure differential -> roll > 0 without pitch.
void move_y(TestHarness& runner, std::float64_t hover_rpm);

// Scenario F: positive mean (+5 degrees) and negative differential -> pitch > 0 and roll < 0.
void combined(TestHarness& runner, std::float64_t hover_rpm);

}
