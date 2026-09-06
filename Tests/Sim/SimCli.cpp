/*
Filename: Tests/Sim/SimCli.cpp
Description: Implementation of the simulation test suite command line : --target
parsing, scenario lookup/selection and the default all-scenarios fill.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SimCli;

import std;

import Scenarios;
import TestHarness;

namespace
{
    constexpr std::string_view kTargetPrefix = "--target=";

    /*
    Reports an unknown scenario argument, prints the usage and returns the
    matching exit code.
    */
    std::int32_t reject_argument(std::string_view argument, std::string_view executableName)
    {
        std::cout << "Error: unknown scenario \"" << argument << "\"." << std::endl;
        std::cout << std::endl;
        sim::test::ScenarioCatalog::print_usage(executableName);
        return 2;
    }

    /* Handles the --target=<name> option; returns 0 when accepted, 2 when rejected. */
    std::int32_t handle_target_option(std::string_view      argument,
                                      std::string_view      executableName,
                                      sim::test::RunTarget& parsedTarget)
    {
        const std::string_view                    targetName = argument.substr(kTargetPrefix.size());
        const std::optional<sim::test::RunTarget> target = sim::test::parse_run_target(targetName);

        if (!target.has_value() || *target != sim::test::RunTarget::Simulation) {
            std::cout << "Error: target \"" << targetName
                      << "\" is not simulation-compatible." << std::endl;
            std::cout << std::endl;
            sim::test::ScenarioCatalog::print_usage(executableName);
            return 2;
        }
        parsedTarget = *target;
        return 0;
    }

    /* Selects every catalog scenario when no explicit one was requested. */
    void select_all_scenarios(sim::test::ScenarioSelection& selected)
    {
        for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
            selected.entries[selected.count] = &entry;
            ++selected.count;
        }
    }
}

namespace sim::test {

std::int32_t select_scenarios(int argc, char* argv[], std::string_view executableName,
                              ScenarioSelection& selected, sim::test::RunTarget& parsedTarget)
{
    for (std::int32_t index = 1; index < argc; ++index) {
        const char* rawArgument = argv[index] != nullptr ? argv[index] : "";
        const std::string_view argument{rawArgument};

        if (argument == "-h" || argument == "--help") {
            sim::test::ScenarioCatalog::print_usage(executableName);
            return 0;
        }

        if (argument.starts_with(kTargetPrefix)) {
            const std::int32_t status = handle_target_option(argument, executableName, parsedTarget);
            if (status != 0) {
                return status;
            }
            continue;
        }

        const sim::test::ScenarioEntry* entry = sim::test::ScenarioCatalog::find(argument);
        if (entry == nullptr) {
            return reject_argument(argument, executableName);
        }
        if (selected.count >= selected.entries.size()) {
            std::cout << "Error: too many scenarios requested." << std::endl;
            return 2;
        }

        selected.entries[selected.count] = entry;
        ++selected.count;
    }

    if (selected.count == 0) {
        select_all_scenarios(selected);
    }

    return -1;
}

}
