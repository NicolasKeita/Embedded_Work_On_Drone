/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Cli.cpp
Description: Command-line parsing and usage printing of the Monte-Carlo campaign.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

namespace sim::monte_carlo {

namespace {
    constexpr std::string_view kScenarioPrefix = "--scenario=";

    [[nodiscard]] std::expected<std::uint64_t, std::errc> parse_unsigned(std::string_view text)
    {
        std::uint64_t                value = 0;
        const std::from_chars_result result = std::from_chars(text.data(), text.data() + text.size(), value);

        if (result.ec != std::errc{}) {
            return std::unexpected(result.ec);
        }
        if (result.ptr != text.data() + text.size()) {
            return std::unexpected(std::errc::invalid_argument);
        }
        return value;
    }

    /* Reads the operand of "--option"; unexpected when the operand is missing. */
    [[nodiscard]] std::expected<std::string, std::string> value_of(int argc, char* argv[],
                                                                    int& index, std::string_view label)
    {
        if (index + 1 >= argc) {
            return std::unexpected(std::format("Error: missing value for {}.", label));
        }
        ++index;
        return std::string{argv[index] != nullptr ? argv[index] : ""};
    }

    /* Reports an option error and marks the campaign options invalid. */
    void report_error(CliOptions& options, const std::string& message)
    {
        std::cerr << message << std::endl;
        options.invalid = true;
    }

    /* Applies an unsigned numeric option, converted to the field width. */
    template<typename Target>
    [[nodiscard]] std::expected<void, std::string> apply_unsigned(Target& field,
                                                                   const std::string& raw,
                                                                   std::string_view label)
    {
        const std::expected<std::uint64_t, std::errc> value = parse_unsigned(raw);

        if (!value.has_value()) {
            return std::unexpected(std::format("Error: invalid value for {}: {}", label, raw));
        }
        field = static_cast<Target>(value.value());
        return {};
    }
}

/* Parses the SIL_MONTE_CARLO command line into CliOptions. */
CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index] != nullptr ? argv[index] : "";
        if (argument == "-h" || argument == "--help") {
            options.help = true;
            return options;
        }
        if (argument == "--seed") {
            const std::expected<std::string, std::string> raw = value_of(argc, argv, index, "--seed");
            if (!raw.has_value()) { report_error(options, raw.error()); return options; }
            const auto applied = apply_unsigned(options.seed, raw.value(), "--seed");
            if (!applied.has_value()) { report_error(options, applied.error()); }
            continue;
        }
        if (argument == "--runs") {
            const std::expected<std::string, std::string> raw = value_of(argc, argv, index, "--runs");
            if (!raw.has_value()) { report_error(options, raw.error()); return options; }
            const auto applied = apply_unsigned(options.runs, raw.value(), "--runs");
            if (!applied.has_value()) { report_error(options, applied.error()); }
            continue;
        }
        if (argument == "--scenario") {
            const std::expected<std::string, std::string> raw = value_of(argc, argv, index, "--scenario");
            if (!raw.has_value()) { report_error(options, raw.error()); return options; }
            options.scenario = raw.value();
            continue;
        }
        if (argument.starts_with(kScenarioPrefix)) {
            options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
            continue;
        }
        report_error(options, std::format("Error: unknown argument \"{}\".", argument));
        return options;
    }
    return options;
}

/* Prints the SIL_MONTE_CARLO usage banner. */
void print_usage(std::string_view executableName)
{
    std::cout << "SIL Monte-Carlo: accelerated statistical batch simulation." << std::endl;
    std::cout << "Usage: " << executableName << " [--seed <n>] [--runs <n>] [--scenario <template>]" << std::endl;
    std::cout << "  --seed <n>       Master RNG seed (default 42)." << std::endl;
    std::cout << "  --runs <n>       Number of iterations (default 50)." << std::endl;
    std::cout << "  --scenario <id>  Scenario to stress-test (default: NOMINAL-001)" << std::endl;
    std::cout << "                     Examples: NOMINAL-001, NOMINAL-008, NOMINAL-009, NOMINAL-010" << std::endl;
    std::cout << "  -h, --help       Show this help." << std::endl;
}

}
