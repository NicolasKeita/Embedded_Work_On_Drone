/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Parse.cpp
Description: Option-parsing helpers of the HIL runner CLI : numeric parsing and per-field apply.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

namespace sim::hil {

namespace {
    template<typename Number>
    [[nodiscard]] std::expected<Number, std::errc> parse_number(std::string_view text)
    {
        Number                       value{};
        const std::from_chars_result result = std::from_chars(text.data(), text.data() + text.size(), value);

        if (result.ec != std::errc{}) {
            return std::unexpected(result.ec);
        }
        if (result.ptr != text.data() + text.size()) {
            return std::unexpected(std::errc::invalid_argument);
        }
        return value;
    }

    [[nodiscard]] std::expected<std::string, std::string> value_of(int argc, char** argv, int index,
                                                                    std::string_view label)
    {
        if (index + 1 >= argc) { return std::unexpected(std::format("missing value for {}", label)); }
        return std::string{argv[index + 1]};
    }
}

/* Applies a string-valued option to the requested field. */
std::expected<void, std::string> apply_string_option(std::string& field, int argc, char** argv,
                                                     int& i, std::string_view label)
{
    const auto value = value_of(argc, argv, i, label);

    if (!value.has_value()) { return std::unexpected(value.error()); }
    field = value.value();
    ++i;
    return {};
}

/* Applies a float-valued option to the requested field. */
std::expected<void, std::string> apply_float_argument(std::optional<std::float64_t>& field, int argc,
                                                      char** argv, int& i, std::string_view label)
{
    const auto raw = value_of(argc, argv, i, label);

    if (!raw.has_value()) { return std::unexpected(raw.error()); }
    const std::expected<std::float64_t, std::errc> value = parse_number<std::float64_t>(raw.value());
    if (!value.has_value()) {
        return std::unexpected(std::format("invalid numeric value for {}: {}", label, raw.value()));
    }
    field = value.value();
    ++i;
    return {};
}

/* Applies the --seed option to the campaign options. */
std::expected<void, std::string> apply_seed_argument(HilCliOptions& options, int argc,
                                                     char** argv, int& i)
{
    const auto raw = value_of(argc, argv, i, "--seed");

    if (!raw.has_value()) { return std::unexpected(raw.error()); }
    const std::expected<std::uint64_t, std::errc> value = parse_number<std::uint64_t>(raw.value());
    if (!value.has_value()) { return std::unexpected(std::format("invalid seed value: {}", raw.value())); }
    options.seed = value.value();
    ++i;
    return {};
}

}
