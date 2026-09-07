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
    std::from_chars, or std::errc when the value is missing or malformed.
    */
    template<typename Number>
    [[nodiscard]] std::expected<Number, std::errc> parse_number(std::string_view text)
    {
        Number value{};
        const std::from_chars_result result =
            std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{}) {
            return std::unexpected(result.ec);
        }
        if (result.ptr != text.data() + text.size()) {
            return std::unexpected(std::errc::invalid_argument);
        }
        return value;
    }

    /*
    Reads the value of a "--option value" pair and reports a missing value.
    */
    [[nodiscard]] std::expected<std::string, std::string> value_of(int argc, char** argv, int index,
                                                                    std::string_view label)
    {
        if (index + 1 >= argc) {
            return std::unexpected(std::format("missing value for {}", label));
        }
        return std::string{argv[index + 1]};
    }

    [[nodiscard]] std::expected<void, std::string> apply_float_option(std::optional<std::float64_t>& field,
                                                                       const std::string& raw,
                                                                       std::string_view label)
    {
        const std::expected<std::float64_t, std::errc> value = parse_number<std::float64_t>(raw);
        if (!value.has_value()) {
            return std::unexpected(std::format("invalid numeric value for {}: {}", label, raw));
        }
        field = value.value();
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> apply_seed_option(HilCliOptions& options,
                                                                      const std::string& raw)
    {
        const std::expected<std::uint64_t, std::errc> value = parse_number<std::uint64_t>(raw);
        if (!value.has_value()) {
            return std::unexpected(std::format("invalid seed value: {}", raw));
        }
        options.seed = value.value();
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> apply_string_option(std::string& field,
                                                                        int argc,
                                                                        char** argv,
                                                                        int& i,
                                                                        std::string_view label)
    {
        const auto value = value_of(argc, argv, i, label);
        if (!value.has_value()) { return std::unexpected(value.error()); }
        field = value.value();
        ++i;
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> apply_float_argument(std::optional<std::float64_t>& field,
                                                                          int argc,
                                                                          char** argv,
                                                                          int& i,
                                                                          std::string_view label)
    {
        const auto value = value_of(argc, argv, i, label);
        if (!value.has_value()) { return std::unexpected(value.error()); }
        ++i;
        const auto applied = apply_float_option(field, value.value(), label);
        if (!applied.has_value()) { return std::unexpected(applied.error()); }
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> apply_seed_argument(HilCliOptions& options,
                                                                         int argc,
                                                                         char** argv,
                                                                         int& i)
    {
        const auto value = value_of(argc, argv, i, "--seed");
        if (!value.has_value()) { return std::unexpected(value.error()); }
        ++i;
        const auto applied = apply_seed_option(options, value.value());
        if (!applied.has_value()) { return std::unexpected(applied.error()); }
        return {};
    }

    [[nodiscard]] std::expected<void, std::string> apply_value_option(HilCliOptions& options,
                                                                       const std::string& arg,
                                                                       int argc,
                                                                       char** argv,
                                                                       int& i)
    {
        if (arg == "--scenario") {
            return apply_string_option(options.scenario_id, argc, argv, i, "--scenario");
        }
        else if (arg == "--interface") {
            return apply_string_option(options.interface_name, argc, argv, i, "--interface");
        }
        else if (arg == "--duration") {
            return apply_float_argument(options.duration, argc, argv, i, "--duration");
        }
        else if (arg == "--telemetry-period") {
            return apply_float_argument(options.telemetry_period, argc, argv, i, "--telemetry-period");
        }
        else if (arg == "--seed") {
            return apply_seed_argument(options, argc, argv, i);
        }
        else if (arg == "--noise") {
            return apply_float_argument(options.noise, argc, argv, i, "--noise");
        }
        else if (arg == "--clock") {
            return apply_string_option(options.clock_name, argc, argv, i, "--clock");
        }
        else if (arg == "--deadline") {
            return apply_string_option(options.deadline_name, argc, argv, i, "--deadline");
        }
        else {
            return std::unexpected(std::format("unknown option: {}", arg));
        }
        return {};
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
            const auto result = apply_value_option(options, arg, argc, argv, i);
            if (!result.has_value()) {
                std::cerr << result.error() << "\n";
                options.invalid = true;
            }
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
