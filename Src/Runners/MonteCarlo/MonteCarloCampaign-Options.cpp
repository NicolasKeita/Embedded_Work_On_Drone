/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Options.cpp
Description: Option dispatch and value parsing of the Monte-Carlo campaign command line.

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

    /* Parses "--option <n>" into the target field; invalid values are reported but parsing continues. */
    template<typename Target>
    bool parse_unsigned_option(CliOptions& options, int argc, char* argv[], int& index,
                               Target& field, std::string_view label)
    {
        const std::expected<std::string, std::string> raw = value_of(argc, argv, index, label);

        if (!raw.has_value()) { report_error(options, raw.error()); return false; }
        const std::expected<std::uint64_t, std::errc> value = parse_unsigned(raw.value());
        if (!value.has_value()) {
            report_error(options, std::format("Error: invalid value for {}: {}", label, raw.value()));
            return true;
        }
        field = static_cast<Target>(value.value());
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

/* Applies one recognized option; returns false when parsing must stop. */
bool apply_option(CliOptions& options, int argc, char* argv[], int& index, std::string_view argument)
{
    if (argument == "-v" || argument == "--verbose") {
        options.verbose = true;
        return true;
    }
    if (argument == "--seed") {
        return parse_unsigned_option(options, argc, argv, index, options.seed, "--seed");
    }
    if (argument == "--runs") {
        return parse_unsigned_option(options, argc, argv, index, options.runs, "--runs");
    }
    if (argument == "--scenario") {
        return parse_string_option(options, argc, argv, index, options.scenario, "--scenario");
    }
    if (argument.starts_with(kScenarioPrefix)) {
        options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
        return true;
    }
    if (argument == "--output-csv") {
        return parse_string_option(options, argc, argv, index, options.output_csv, "--output-csv");
    }
    if (argument == "--output-json") {
        return parse_string_option(options, argc, argv, index, options.output_json, "--output-json");
    }
    if (argument.starts_with(kOutputCsvPrefix)) {
        options.output_csv = std::string{argument.substr(kOutputCsvPrefix.size())};
        return true;
    }
    if (argument.starts_with(kOutputJsonPrefix)) {
        options.output_json = std::string{argument.substr(kOutputJsonPrefix.size())};
        return true;
    }
    report_error(options, std::format("Error: unknown argument \"{}\".", argument));
    return false;
}

}
