/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Cli.cpp
Description: Command-line parsing of the Monte-Carlo campaign.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

namespace sim::monte_carlo {

namespace {
    constexpr std::string_view kScenarioPrefix = "--scenario=";
    constexpr std::string_view kOutputCsvPrefix = "--output-csv=";
    constexpr std::string_view kOutputJsonPrefix = "--output-json=";

    [[nodiscard]] std::expected<std::uint64_t, std::errc> parse_unsigned(std::string_view text)
    {
        std::uint64_t                value = 0;
        const std::from_chars_result result = std::from_chars(text.data(), text.data() + text.size(), value);

        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
            return std::unexpected(result.ec != std::errc{} ? result.ec : std::errc::invalid_argument);
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

    /* Parses "--option <n>" into the target field; returns false to stop parsing. */
    template<typename Target>
    bool parse_unsigned_option(CliOptions& options, int argc, char* argv[], int& index,
                               Target& field, std::string_view label)
    {
        const std::expected<std::string, std::string> raw = value_of(argc, argv, index, label);

        if (!raw.has_value()) { report_error(options, raw.error()); return false; }
        const auto applied = apply_unsigned(field, raw.value(), label);
        if (!applied.has_value()) { report_error(options, applied.error()); }
        return true;
    }

    /* Parses "--option <value>" into the target string field; returns false to stop parsing. */
    bool parse_string_option(CliOptions& options, int argc, char* argv[], int& index,
                             std::string& field, std::string_view label)
    {
        const std::expected<std::string, std::string> raw = value_of(argc, argv, index, label);
        if (!raw.has_value()) { report_error(options, raw.error()); return false; }
        field = raw.value();
        return true;
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
        if (argument == "-v" || argument == "--verbose") {
            options.verbose = true;
            continue;
        }
        if (argument == "--seed") {
            if (!parse_unsigned_option(options, argc, argv, index, options.seed, "--seed")) { return options; }
            continue;
        }
        if (argument == "--runs") {
            if (!parse_unsigned_option(options, argc, argv, index, options.runs, "--runs")) { return options; }
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
        if (argument == "--output-csv") {
            if (!parse_string_option(options, argc, argv, index, options.output_csv, "--output-csv")) { return options; }
            continue;
        }
        if (argument == "--output-json") {
            if (!parse_string_option(options, argc, argv, index, options.output_json, "--output-json")) { return options; }
            continue;
        }
        if (argument.starts_with(kOutputCsvPrefix)) {
            options.output_csv = std::string{argument.substr(kOutputCsvPrefix.size())};
            continue;
        }
        if (argument.starts_with(kOutputJsonPrefix)) {
            options.output_json = std::string{argument.substr(kOutputJsonPrefix.size())};
            continue;
        }
        report_error(options, std::format("Error: unknown argument \"{}\".", argument));
        return options;
    }
    return options;
}

}
