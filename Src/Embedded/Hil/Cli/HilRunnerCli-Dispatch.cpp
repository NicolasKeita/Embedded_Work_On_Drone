/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Dispatch.cpp
Description: Value-option dispatching and parse_hil_cli() of the HIL runner CLI.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

namespace sim::hil {

namespace {
    /* Applies one value option and reports missing or invalid arguments. */
    [[nodiscard]] std::expected<void, std::string> apply_value_option(HilCliOptions& options,
                                                                       const std::string& arg,
                                                                       int argc,
                                                                       char** argv,
                                                                       int& i)
    {
        if (arg == "--scenario") {
            return apply_string_option(options.scenario_id, argc, argv, i, "--scenario");
        }
        if (arg == "--interface") {
            return apply_string_option(options.interface_name, argc, argv, i, "--interface");
        }
        if (arg == "--config") {
            return apply_string_option(options.config_path, argc, argv, i, "--config");
        }
        if (arg == "--simulation-config") {
            return apply_string_option(options.simulation_config_path, argc, argv, i, "--simulation-config");
        }
        if (arg == "--scenarios-dir") {
            return apply_string_option(options.scenarios_directory, argc, argv, i, "--scenarios-dir");
        }
        if (arg == "--config-output") {
            return apply_string_option(options.config_output_path, argc, argv, i, "--config-output");
        }
        if (arg == "--hardware-config") {
            const auto result = apply_string_option(options.hardware_config_path, argc, argv, i,
                                                    "--hardware-config");
            if (!result.has_value()) {
                return result;
            }
            if (options.hardware_config_path.empty()) {
                return std::unexpected("--hardware-config requires a nonempty path");
            }
            return {};
        }
        if (arg == "--duration") {
            return apply_float_argument(options.duration, argc, argv, i, "--duration");
        }
        if (arg == "--telemetry-period") {
            return apply_float_argument(options.telemetry_period, argc, argv, i, "--telemetry-period");
        }
        if (arg == "--seed") {
            return apply_seed_argument(options, argc, argv, i);
        }
        if (arg == "--noise") {
            return apply_float_argument(options.noise, argc, argv, i, "--noise");
        }
        if (arg == "--deadline") {
            return apply_string_option(options.deadline_name, argc, argv, i, "--deadline");
        }
        return std::unexpected(std::format("unknown option: {}", arg));
    }
}

HilCliOptions parse_hil_cli(int argc, char** argv)
{
    HilCliOptions options{};

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--list") {
            options.list_only = true;
        }
        else if (arg == "--selftest") {
            options.selftest = true;
        }
        else if (arg == "--help" || arg == "-h") {
            options.help = true;
            return options;
        }
        else {
            const auto result = apply_value_option(options, arg, argc, argv, i);
            if (!result.has_value()) {
                std::cerr << result.error() << "\n";
                options.invalid = true;
            }
        }
    }
    return options;
}

}
