/*
Filename: Tests/Sim/SimCli.cppm
Description: Command-line interface of the simulation test suite : scenario selection
and --target parsing for test_simulation.
Exports:
    kMaxSelectedScenarios,
    struct ScenarioSelection,
    select_scenarios()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SimCli;

import std;

import Scenarios;
import TestHarness;

export namespace sim::test {

inline constexpr std::size_t kMaxSelectedScenarios = 10;

struct ScenarioSelection {
    std::array<const sim::test::ScenarioEntry*, kMaxSelectedScenarios> entries{};
    std::size_t                                                        count = 0;
};

/*
Resolves scenario selection from the command line; returns a code >= 0 to
terminate immediately, -1 to continue with the selected scenarios.
*/
std::int32_t select_scenarios(int argc, char* argv[], std::string_view executableName,
                              ScenarioSelection& selected, sim::test::RunTarget& parsedTarget);

}
