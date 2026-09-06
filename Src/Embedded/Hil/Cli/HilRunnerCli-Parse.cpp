/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Parse.cpp
Description: hil_runner command-line parsing, usage and scenario listing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

import HilScenarios;

namespace sim::hil {

namespace {
    std::optional<std::string> value_of(int argc, char** argv, int& i, std::string_view label)
    {
        if (i + 1 >= argc) {
            std::cerr << "missing value for " << label << "\n";
            return std::nullopt;
        }
        return std::string{argv[++i]};
    }

    bool apply_value_option(HilCliOptions& options, const std::string& arg, int argc, char** argv, int& i)
    {
        if (arg == "--scenario") {
            if (const auto v = value_of(argc, argv, i, "--scenario")) { options.scenario_id = *v; }
        }
        else if (arg == "--duration") {
            if (const auto v = value_of(argc, argv, i, "--duration")) { options.duration = std::stod(*v); }
        }
        else if (arg == "--telemetry-period") {
            if (const auto v = value_of(argc, argv, i, "--telemetry-period")) {
                options.telemetry_period = std::stod(*v);
            }
        }
        else if (arg == "--seed") {
            if (const auto v = value_of(argc, argv, i, "--seed")) { options.seed = std::stoull(*v); }
        }
        else if (arg == "--noise") {
            if (const auto v = value_of(argc, argv, i, "--noise")) { options.noise = std::stod(*v); }
        }
        else if (arg == "--clock") {
            if (const auto v = value_of(argc, argv, i, "--clock")) { options.clock_name = *v; }
        }
        else if (arg == "--deadline") {
            if (const auto v = value_of(argc, argv, i, "--deadline")) { options.deadline_name = *v; }
        }
        else {
            return false;
        }
        return true;
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
        else if (arg == "--no-realtime") {
            options.no_realtime = true;
        }
        else if (arg == "--help" || arg == "-h") {
            options.help = true;
            return options;
        }
        else {
            apply_value_option(options, arg, argc, argv, i);
        }
    }
    return options;
}

void print_hil_usage(std::string_view name)
{
    std::cout << "HIL runner: real-time closed-loop mission against the FC target.\n";
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "  --scenario <id>         functional scenario ID (default NOM-001_StationKeeping)\n";
    std::cout << "  --duration <s>          override mission duration in seconds\n";
    std::cout << "  --telemetry-period <s>  override human-readable report period (default 1 s)\n";
    std::cout << "  --seed <n>              random seed\n";
    std::cout << "  --noise <stddev>        sensor measurement noise std dev in meters\n";
    std::cout << "  --no-realtime           run as fast as possible (no wall-clock pacing)\n";
    std::cout << "  --clock <Monotonic|Fast> wall-clock source (default Monotonic; Fast for tests)\n";
    std::cout << "  --deadline <Warn|Fail|Abort>  deadline-miss policy (default Warn)\n";
    std::cout << "  --list                  list available scenarios\n";
    std::cout << "  --target <sil|hil>      execution target tag for the report (default hil)\n";
}

void list_hil_scenarios()
{
    std::cout << "HIL scenarios:\n";
    for (const sim::hil::HilScenarioRecord& s : sim::hil::HilScenarioCatalog::all()) {
        std::cout << "  " << s.id << " : " << s.description << " [HIL]\n";
    }
}

}