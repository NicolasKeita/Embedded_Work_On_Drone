/*
Filename: Src/Runners/Sil/SilRunnerCli.cpp
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
Applies the --telemetry-period value: parses the given text as a strictly
positive duration in seconds, marking the options invalid on malformed input.
*/
void apply_telemetry_period(CliOptions& options, std::string_view value)
{
    double         parsed = 0.0;
    const char*    first  = value.data();
    const char*    last   = first + value.size();
    const auto     result = std::from_chars(first, last, parsed);

    if (result.ec != std::errc{} || result.ptr != last || parsed <= 0.0) {
        std::cerr << "Error: invalid value for --telemetry-period \"" << value << "\"." << std::endl;
        options.invalid = true;
        return;
    }
    options.telemetry_period = static_cast<std::float64_t>(parsed);
}

/*
Parses the SIL_RUNNER command line: --scenario <id> (or --scenario=<id>),
--telemetry-period <s> (or --telemetry-period=<s>), --all, --verbose/-v and
-h/--help. Any other argument, or --all combined with --scenario, marks the
options invalid.
*/
CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions                 options;
    constexpr std::string_view kScenarioPrefix        = "--scenario=";
    constexpr std::string_view kTelemetryPeriodPrefix = "--telemetry-period=";

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
        if (argument == "--telemetry-period") {
            if (index + 1 >= argc) {
                std::cerr << "Error: missing value for --telemetry-period." << std::endl;
                options.invalid = true;
                return options;
            }
            ++index;
            apply_telemetry_period(options, argv[index] != nullptr ? argv[index] : "");
            if (options.invalid) {
                return options;
            }
            continue;
        }
        if (argument.starts_with(kScenarioPrefix)) {
            options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
            continue;
        }
        if (argument.starts_with(kTelemetryPeriodPrefix)) {
            apply_telemetry_period(options, argument.substr(kTelemetryPeriodPrefix.size()));
            if (options.invalid) {
                return options;
            }
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
