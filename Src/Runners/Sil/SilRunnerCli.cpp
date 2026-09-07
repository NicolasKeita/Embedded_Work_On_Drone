/*
Filename: Src/Runners/Sil/SilRunnerCli-Parse.cpp
Description: Option parsing and usage printing of the SIL_RUNNER command line.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerCli;

import std;

import Scenarios;
import SilScenarios;

namespace sim::test::sil {

/*
Parses the SIL_RUNNER command line: --scenario <id> (or --scenario=<id>),
--all, --verbose/-v and -h/--help. Any other argument, or --all combined with
--scenario, marks the options invalid.
*/
CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions                 options;
    constexpr std::string_view kScenarioPrefix = "--scenario=";

    for (int index = 1; index < argc; ++index) {
        const char*            raw = argv[index] != nullptr ? argv[index] : "";
        const std::string_view argument{raw};

        if (argument == "-h" || argument == "--help") {
            options.help = true;
            return options;
        }
        if (argument == "-a" || argument == "--all") {
            options.all = true;
            continue;
        }
        if (argument == "-v" || argument == "--verbose") {
            options.verbose = true;
            continue;
        }
        if (argument == "--scenario") {
            if (index + 1 >= argc) {
                std::cerr << "Error: missing value for --scenario." << std::endl;
                options.invalid = true;
                return options;
            }
            ++index;
            options.scenario = argv[index] != nullptr ? argv[index] : "";
            continue;
        }
        if (argument.starts_with(kScenarioPrefix)) {
            options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
            continue;
        }
        std::cerr << "Error: unknown argument \"" << argument << "\"." << std::endl;
        options.invalid = true;
        return options;
    }
    if (options.all && options.scenario.has_value()) {
        std::cerr << "Error: --all cannot be combined with --scenario." << std::endl;
        options.invalid = true;
        return options;
    }
    return options;
}

/* Prints the usage banner and the full deterministic scenario catalog. */
void print_usage(std::string_view executableName)
{
    std::cout << "SIL runner: deterministic software-in-the-loop execution at maximum CPU speed." << std::endl;
    std::cout << "Usage: " << executableName << " [--scenario <id>] [--all] [-v | --verbose]" << std::endl;
    std::cout << "  With no argument, this help is printed and nothing is executed." << std::endl;
    std::cout << "  --scenario <id>  Run one scenario from the catalog below." << std::endl;
    std::cout << "  --all            Run the full deterministic sweep (every scenario)." << std::endl;
    std::cout << "  -v, --verbose    Per-step telemetry logging." << std::endl;
    std::cout << "  -h, --help       Show this help." << std::endl;
    std::cout << std::endl;
    std::cout << "SIL engine scenarios:" << std::endl;
    for (const SilScenarioEntry& entry : sil_scenarios()) {
        std::cout << "  " << entry.id << " : " << entry.description << std::endl;
    }
    std::cout << "Physics and autonomous scenarios:" << std::endl;
    for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
        std::cout << "  " << entry.id << " : " << entry.description << std::endl;
    }
}

}
