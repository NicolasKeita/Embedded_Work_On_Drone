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

using ScenarioFn = void (*)(TestHarness&, double);

struct ScenarioEntry {
    char key;
    std::string_view description;
    ScenarioFn run;
};

class ScenarioCatalog {
public:
    [[nodiscard]] static std::span<const ScenarioEntry> all() noexcept;
    [[nodiscard]] static const ScenarioEntry* find(char argument) noexcept;
    static void print_usage(std::string_view executableName);

private:
    static const std::array<ScenarioEntry, 10> scenarios_;
};

} // namespace sim::test
