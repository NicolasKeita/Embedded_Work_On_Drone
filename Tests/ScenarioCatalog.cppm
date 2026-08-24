/*
Filename: Tests/ScenarioCatalog.cppm
Description: Public interface of the launchable scenario registry and command-line usage.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ScenarioCatalog;

import std;

import TestHarness;

export namespace sim::test {

using ScenarioFn = std::function<void(TestHarness&, double)>;

struct ScenarioEntry {
    char key;
    std::string_view description;
    ScenarioFn run;
};

[[nodiscard]] std::span<const ScenarioEntry> get_scenarios() noexcept;
[[nodiscard]] const ScenarioEntry* find_scenario(char argument) noexcept;
void print_usage(std::string_view executable_name);

} // namespace sim::test
