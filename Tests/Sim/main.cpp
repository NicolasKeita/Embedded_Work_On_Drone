/*
Filename: Tests/Sim/main.cpp
Description: Entry point running the deterministic validation scenarios at 100 Hz.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import Scenarios;
import TestHarness;

namespace
{
    constexpr std::size_t kMaxSelectedScenarios = 10;

    struct ScenarioSelection {
        std::array<const sim::test::ScenarioEntry*, kMaxSelectedScenarios> entries{};
        std::size_t count = 0;
    };

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

    /*
    Resolves scenario selection from the command line; returns a code >= 0 to
    terminate immediately, -1 to continue with the selected scenarios.
    */
    std::int32_t select_scenarios(int argc, char* argv[], std::string_view executableName,
                        ScenarioSelection& selected, sim::test::RunTarget& parsedTarget)
    {
        constexpr std::string_view targetPrefix = "--target=";

        for (std::int32_t index = 1; index < argc; ++index) {
            const char* rawArgument = argv[index] != nullptr ? argv[index] : "";
            const std::string_view argument{rawArgument};

            if (argument == "-h" || argument == "--help") {
                sim::test::ScenarioCatalog::print_usage(executableName);
                return 0;
            }

            if (argument.starts_with(targetPrefix)) {
                const std::string_view targetName = argument.substr(targetPrefix.size());
                const std::optional<sim::test::RunTarget> target =
                    sim::test::parse_run_target(targetName);
                if (!target.has_value() || *target != sim::test::RunTarget::Simulation) {
                    std::cout << "Error: target \"" << targetName
                              << "\" is not simulation-compatible." << std::endl;
                    std::cout << std::endl;
                    sim::test::ScenarioCatalog::print_usage(executableName);
                    return 2;
                }
                parsedTarget = *target;
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
            for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
                selected.entries[selected.count] = &entry;
                ++selected.count;
            }
        }

        return -1;
    }
}

/*
Entry point: a single runner is shared by all scenarios so that the failure
counter is accumulated over the whole validation.
*/
int main(int argc, char* argv[])
{
    const std::string_view executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "test_simulation";
    ScenarioSelection      selected;
    sim::test::RunTarget   parsedTarget = sim::test::RunTarget::Simulation;
    const std::int32_t     earlyStatus = select_scenarios(argc, argv, executableName, selected, parsedTarget);

    if (earlyStatus >= 0) {
        return static_cast<int>(earlyStatus);
    }

    const Aircraft reference;
    const std::float64_t hoverRpm = reference.hover_rpm();

    sim::test::TestHarness runner{sim::test::HarnessConfig{
        .dt = 0.01, .log_interval_steps = 100, .target = parsedTarget}};

    std::cout << "=== Physical simulator validation (Heliblade-like) ===" << std::endl;
    std::cout << "Theoretical hover RPM: "
              << std::fixed << std::setprecision(1) << hoverRpm << " rpm" << std::endl;
    std::cout << "Deterministic single-thread loop at 100 Hz (dt = 0.01 s)." << std::endl;

    for (std::size_t index = 0; index < selected.count; ++index) {
        selected.entries[index]->run(runner, hoverRpm);
    }

    if (runner.passed()) {
        std::cout << "\n>>> All launched scenarios passed (PASS)." << std::endl;
        return 0;
    }

    std::cout << "\n>>> " << runner.failure_count()
              << " verification(s) failed (FAIL)." << std::endl;
    return 1;
}
