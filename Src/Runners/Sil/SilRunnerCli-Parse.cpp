/*
Filename: Src/Runners/Sil/SilRunnerCli-Parse.cpp
Description: Option parsing of the SIL_RUNNER command line.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerCli;

import std;

namespace sim::test::sil {

/*
Applies the --telemetry-period value: parses the given text as a strictly
positive duration in seconds, marking the options invalid on malformed input.
*/
void apply_telemetry_period(CliOptions& options, std::string_view value)
{
    std::float64_t parsed = 0.0;
    const char* first = value.data();
    const char* last = first + value.size();
    const auto  result = std::from_chars(first, last, parsed);

    if (result.ec != std::errc{} || result.ptr != last || !std::isfinite(parsed) || parsed <= 0.0 || parsed > 86400.0) {
        std::cerr << "Error: invalid value for --telemetry-period \"" << value << "\"." << std::endl;
        options.invalid = true;
        return;
    }
    options.telemetry_period = static_cast<std::float64_t>(parsed);
}

namespace {

    constexpr std::string_view kScenarioPrefix = "--scenario=";
    constexpr std::string_view kTelemetryPeriodPrefix = "--telemetry-period=";

    /* Reports a missing operand for the given option and stops parsing. */
    bool missing_option_value(CliOptions& options, std::string_view label)
    {
        std::cerr << "Error: missing value for " << label << "." << std::endl;
        options.invalid = true;
        return false;
    }

    /* Applies one recognized option; returns false when parsing must stop. */
    bool apply_option(CliOptions& options, int argc, char* argv[], int& index, std::string_view argument)
    {
        const std::array<std::pair<std::string_view, std::string*>, 4> paths{{
            {"--config", &options.config_path},
            {"--simulation-config", &options.simulation_config_path},
            {"--scenarios-dir", &options.scenarios_dir},
            {"--config-output", &options.config_output},
        }};
        for (const auto& path : paths) {
            if (argument == path.first) {
                if (index + 1 >= argc) {
                    return missing_option_value(options, path.first);
                }
                ++index;
                *path.second = argv[index] != nullptr ? argv[index] : "";
                if (path.second->empty() || path.second->starts_with("--")) {
                    return missing_option_value(options, path.first);
                }
                return true;
            }
            const std::string prefix = std::string{path.first} + "=";
            if (argument.starts_with(prefix)) {
                *path.second = argument.substr(prefix.size());
                if (path.second->empty()) {
                    return missing_option_value(options, path.first);
                }
                return true;
            }
        }
        if (argument == "-a" || argument == "--all") {
            options.all = true;
            return true;
        }
        if (argument == "-v" || argument == "--verbose") {
            options.verbose = true;
            return true;
        }
        if (argument == "--scenario") {
            if (index + 1 >= argc) {
                return missing_option_value(options, "--scenario");
            }
            ++index;
            options.scenario = argv[index] != nullptr ? argv[index] : "";
            return true;
        }
        if (argument == "--telemetry-period") {
            if (index + 1 >= argc) {
                return missing_option_value(options, "--telemetry-period");
            }
            ++index;
            apply_telemetry_period(options, argv[index] != nullptr ? argv[index] : "");
            return !options.invalid;
        }
        if (argument.starts_with(kScenarioPrefix)) {
            options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
            return true;
        }
        if (argument.starts_with(kTelemetryPeriodPrefix)) {
            apply_telemetry_period(options, argument.substr(kTelemetryPeriodPrefix.size()));
            return !options.invalid;
        }
        std::cerr << "Error: unknown argument \"" << argument << "\"." << std::endl;
        options.invalid = true;
        return false;
    }
}

/*
Parses the SIL_RUNNER command line: --scenario <id> (or --scenario=<id>),
--telemetry-period <s> (or --telemetry-period=<s>), --all, --verbose/-v and
-h/--help. Any other argument, or --all combined with --scenario, marks the
options invalid.
*/
CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const char*            raw = argv[index] != nullptr ? argv[index] : "";
        const std::string_view argument{raw};

        if (argument == "-h" || argument == "--help") {
            options.help = true;
            return options;
        }
        if (!apply_option(options, argc, argv, index, argument)) {
            return options;
        }
    }
    if (options.all && options.scenario.has_value()) {
        std::cerr << "Error: --all cannot be combined with --scenario." << std::endl;
        options.invalid = true;
        return options;
    }
    return options;
}

}
