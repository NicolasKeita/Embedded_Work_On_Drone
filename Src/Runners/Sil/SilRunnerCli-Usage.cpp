/*
Filename: Src/Runners/Sil/SilRunnerCli-Usage.cpp
Description: Usage printing of the SIL_RUNNER command line with the full scenario catalog.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerCli;

import std;

import Scenarios;
import SilScenarios;

namespace sim::test::sil {

/* Prints every catalog scenario whose standardised ID belongs to the given family. */
void print_scenario_family(std::string_view familyPrefix)
{
    for (const SilScenarioEntry& entry : sil_scenarios()) {
        if (entry.id.starts_with(familyPrefix)) {
            std::cout << "  " << entry.id << " : " << entry.description << std::endl;
        }
    }
    for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
        if (entry.id.starts_with(familyPrefix)) {
            std::cout << "  " << entry.id << " : " << entry.description << std::endl;
        }
    }
}

/* Prints the usage banner and the full scenario catalog grouped by family. */
void print_usage(std::string_view executableName)
{
    std::cout << "SIL runner: deterministic software-in-the-loop execution at maximum CPU speed." << std::endl;
    std::cout << "Usage: " << executableName
              << " [--scenario <id>] [--telemetry-period <s>] [--all] [-v | --verbose]" << std::endl;
    std::cout << "  With no argument, this help is printed and nothing is executed." << std::endl;
    std::cout << "  --scenario <id>         Run one scenario from the catalog below." << std::endl;
    std::cout << "  --telemetry-period <s>  Telemetry table period in seconds (default 1 s)." << std::endl;
    std::cout << "  --all                   Run the full deterministic sweep (every scenario)." << std::endl;
    std::cout << "  -v, --verbose           Per-step telemetry logging." << std::endl;
    std::cout << "  -h, --help              Show this help." << std::endl;
    std::cout << std::endl;
    std::cout << "NOMINAL scenarios:" << std::endl;
    print_scenario_family("NOMINAL");
    std::cout << "FAULT_INJECTOR scenarios:" << std::endl;
    print_scenario_family("FAULT_INJECTOR");
}

}
