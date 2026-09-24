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

    struct ArgCursor {
        int    argc;
        char** argv;
        int&   index;
    };
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
    [[nodiscard]] std::expected<std::string, std::string> value_of(const ArgCursor& cursor, std::string_view label)
    {
        if (cursor.index + 1 >= cursor.argc) {
            return std::unexpected(std::format("Error: missing value for {}.", label));
        }
        ++cursor.index;
        return std::string{cursor.argv[cursor.index] != nullptr ? cursor.argv[cursor.index] : ""};
    }

    /* Reports an option error and marks the campaign options invalid. */
    void report_error(CliOptions& options, const std::string& message)
    {
        std::cerr << message << std::endl;
        options.invalid = true;
    }

    /* Parses "--option <n>" into the target field; invalid values are reported but parsing continues. */
    template<typename Target>
    bool parse_unsigned_option(CliOptions& options, const ArgCursor& cursor, Target& field, std::string_view label)
    {
        const std::expected<std::string, std::string> raw = value_of(cursor, label);

        if (!raw.has_value()) { report_error(options, raw.error()); return false; }
        const std::expected<std::uint64_t, std::errc> value = parse_unsigned(raw.value());
        if (!value.has_value() || *value > std::numeric_limits<Target>::max()) {
            report_error(options, std::format("Error: invalid value for {}: {}", label, raw.value()));
            return true;
        }
        field = static_cast<Target>(value.value());
        return true;
    }

    /* Parses "--option <value>" into the target string field; returns false to stop parsing. */
    bool parse_string_option(CliOptions& options, const ArgCursor& cursor, std::string& field, std::string_view label)
    {
        const std::expected<std::string, std::string> raw = value_of(cursor, label);

        if (!raw.has_value()) { report_error(options, raw.error()); return false; }
        field = raw.value();
        return true;
    }
}

/* Applies one recognized option; returns false when parsing must stop. */
bool apply_option(CliOptions& options, int argc, char* argv[], int& index, std::string_view argument)
{
    ArgCursor cursor{.argc = argc, .argv = argv, .index = index};

    if (argument == "-v" || argument == "--verbose") {
        options.verbose = true;
        return true;
    }
    if (argument == "--seed") {
        options.seed_override = true;
        return parse_unsigned_option(options, cursor, options.seed, "--seed");
    }
    if (argument == "--runs") {
        options.runs_override = true;
        return parse_unsigned_option(options, cursor, options.runs, "--runs");
    }
    if (argument == "--scenario") {
        options.scenario_override = true;
        return parse_string_option(options, cursor, options.scenario, "--scenario");
    }
    if (argument.starts_with(kScenarioPrefix)) {
        options.scenario_override = true;
        options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
        return true;
    }
    if (argument == "--output-csv") {
        return parse_string_option(options, cursor, options.output_csv, "--output-csv");
    }
    if (argument == "--config") {
        return parse_string_option(options, cursor, options.config_path, "--config");
    }
    if (argument == "--sil-config") {
        return parse_string_option(options, cursor, options.sil_config_path, "--sil-config");
    }
    if (argument == "--simulation-config") {
        return parse_string_option(options, cursor, options.simulation_config_path, "--simulation-config");
    }
    if (argument == "--scenarios-dir") {
        return parse_string_option(options, cursor, options.scenarios_directory, "--scenarios-dir");
    }
    if (argument == "--config-output") {
        return parse_string_option(options, cursor, options.config_output_path, "--config-output");
    }
    if (argument == "--output-json") {
        return parse_string_option(options, cursor, options.output_json, "--output-json");
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
