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
    /*
    Exception-free numeric view extraction: returns the argument converted with
    std::from_chars, or std::nullopt when the value is missing or malformed.
    */
    template<typename Number>
    [[nodiscard]] std::optional<Number> parse_number(std::string_view text)
    {
        Number value{};
        const std::from_chars_result result =
            std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
            return std::nullopt;
        }
        return value;
    }

    /*
    Reads the value of a "--option value" pair; reports and flags the options
    as invalid when the value is missing.
    */
    std::optional<std::string> value_of(int argc, char** argv, int& i, std::string_view label, HilCliOptions& options)
    {
        if (i + 1 >= argc) {
            std::cerr << "missing value for " << label << "\n";
            options.invalid = true;
            return std::nullopt;
        }
        return std::string{argv[++i]};
    }

    void apply_float_option(std::optional<std::float64_t>& field, const std::string& raw,
                            std::string_view label, HilCliOptions& options)
    {
        const std::optional<std::float64_t> value = parse_number<std::float64_t>(raw);
        if (!value.has_value()) {
            std::cerr << "invalid numeric value for " << label << ": " << raw << "\n";
            options.invalid = true;
            return;
        }
        field = value;
    }

    void apply_seed_option(HilCliOptions& options, const std::string& raw)
    {
        const std::optional<std::uint64_t> value = parse_number<std::uint64_t>(raw);
        if (!value.has_value()) {
            std::cerr << "invalid seed value: " << raw << "\n";
            options.invalid = true;
            return;
        }
        options.seed = value;
    }

    bool apply_value_option(HilCliOptions& options, const std::string& arg, int argc, char** argv, int& i)
    {
        if (arg == "--scenario") {
            if (const auto v = value_of(argc, argv, i, "--scenario", options)) { options.scenario_id = *v; }
        }
        else if (arg == "--interface") {
            if (const auto v = value_of(argc, argv, i, "--interface", options)) { options.interface_name = *v; }
        }
        else if (arg == "--duration") {
            if (const auto v = value_of(argc, argv, i, "--duration", options)) {
                apply_float_option(options.duration, *v, "--duration", options);
            }
        }
        else if (arg == "--telemetry-period") {
            if (const auto v = value_of(argc, argv, i, "--telemetry-period", options)) {
                apply_float_option(options.telemetry_period, *v, "--telemetry-period", options);
            }
        }
        else if (arg == "--seed") {
            if (const auto v = value_of(argc, argv, i, "--seed", options)) { apply_seed_option(options, *v); }
        }
        else if (arg == "--noise") {
            if (const auto v = value_of(argc, argv, i, "--noise", options)) {
                apply_float_option(options.noise, *v, "--noise", options);
            }
        }
        else if (arg == "--clock") {
            if (const auto v = value_of(argc, argv, i, "--clock", options)) { options.clock_name = *v; }
        }
        else if (arg == "--deadline") {
            if (const auto v = value_of(argc, argv, i, "--deadline", options)) { options.deadline_name = *v; }
        }
        else {
            std::cerr << "unknown option: " << arg << "\n";
            options.invalid = true;
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
        else if (arg == "--selftest") {
            options.selftest = true;
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
    std::cout << "  --scenario <id>         functional scenario ID (default NOMINAL-001)\n";
    std::cout << "  --interface <name>      hardware channel (loopback = in-process HAL, default)\n";
    std::cout << "  --duration <s>          override mission duration in seconds\n";
    std::cout << "  --telemetry-period <s>  override human-readable report period (default 1 s)\n";
    std::cout << "  --seed <n>              random seed\n";
    std::cout << "  --noise <stddev>        sensor measurement noise std dev in meters\n";
    std::cout << "  --no-realtime           run as fast as possible (no wall-clock pacing)\n";
    std::cout << "  --clock <Monotonic|Fast> wall-clock source (default Monotonic; Fast for tests)\n";
    std::cout << "  --deadline <Warn|Fail|Abort>  deadline-miss policy (default Warn)\n";
    std::cout << "  --selftest              run the deterministic HIL validation suite instead of a mission\n";
    std::cout << "  --list                  list available scenarios\n";
}

void list_hil_scenarios()
{
    std::cout << "HIL scenarios:\n";
    for (const sim::hil::HilScenarioRecord& s : sim::hil::HilScenarioCatalog::all()) {
        std::cout << "  " << s.id << " : " << s.description << " [HIL]\n";
    }
}

}
