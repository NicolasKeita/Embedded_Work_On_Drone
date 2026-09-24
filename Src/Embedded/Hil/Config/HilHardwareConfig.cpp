/*
Filename: Src/Embedded/Hil/Config/HilHardwareConfig.cpp
Description: Parsing and validation of file-based HIL ST-LINK serial numbers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilHardwareConfig;

import std;

namespace sim::hil {

namespace {

/* Removes surrounding ASCII whitespace, including CRLF line endings. */
[[nodiscard]] std::string_view trim(std::string_view text) noexcept
{
    constexpr std::string_view whitespace = " \t\r\n\f\v";
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = text.find_last_not_of(whitespace);
    return text.substr(first, last - first + 1);
}

/* Validates the 24 hexadecimal digits emitted by the supported ST-LINK probes. */
[[nodiscard]] bool valid_serial(std::string_view serial) noexcept
{
    return serial.size() == 24 &&
           serial.find_first_not_of("0123456789abcdefABCDEF") == std::string_view::npos;
}

/* Normalizes hexadecimal letters to the uppercase representation used in sysfs. */
[[nodiscard]] std::string normalized_serial(std::string_view serial)
{
    std::string normalized{serial};
    for (auto& character : normalized) {
        if (character >= 'a' && character <= 'f') {
            character = static_cast<char>(character - 'a' + 'A');
        }
    }
    return normalized;
}

/* Validates and stores one known key while rejecting duplicate assignments. */
[[nodiscard]] std::expected<void, std::string>
assign_serial(HilHardwareConfig& config, std::string_view key, std::string_view value)
{
    std::string* destination = nullptr;
    if (key == "fc1_stlink_serial") {
        destination = &config.fc1_stlink_serial;
    } else if (key == "fc2_stlink_serial") {
        destination = &config.fc2_stlink_serial;
    } else {
        return std::unexpected{"unknown key '" + std::string{key} + "'"};
    }
    if (!destination->empty()) {
        return std::unexpected{"duplicate key '" + std::string{key} + "'"};
    }
    if (!valid_serial(value)) {
        return std::unexpected{"'" + std::string{key} +
                               "' must contain exactly 24 hexadecimal digits"};
    }
    *destination = normalized_serial(value);
    return {};
}

/* Adds the source line number to a configuration diagnostic. */
[[nodiscard]] std::string line_error(std::uint64_t line_number, std::string_view message)
{
    return "line " + std::to_string(line_number) + ": " + std::string{message};
}

}

/* Parses uncommented assignments and requires both distinct hardware identities. */
std::expected<HilHardwareConfig, std::string> parse_hil_hardware_config(std::istream& input)
{
    HilHardwareConfig config{};
    std::string line{};
    std::uint64_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        const std::string_view raw_line{line};
        const auto entry = trim(raw_line.substr(0, raw_line.find('#')));
        if (entry.empty()) {
            continue;
        }
        const auto separator = entry.find('=');
        if (separator == std::string_view::npos) {
            return std::unexpected{line_error(line_number, "expected key=value")};
        }
        const auto key = trim(entry.substr(0, separator));
        const auto value = trim(entry.substr(separator + 1));
        const auto result = assign_serial(config, key, value);
        if (!result) {
            return std::unexpected{line_error(line_number, result.error())};
        }
        if (!config.fc1_stlink_serial.empty() &&
            config.fc1_stlink_serial == config.fc2_stlink_serial) {
            return std::unexpected{line_error(line_number,
                                             "FC1 and FC2 ST-LINK serials must be distinct")};
        }
    }
    if (input.bad() || !input.eof()) {
        return std::unexpected{line_error(line_number + 1, "unable to read configuration")};
    }
    if (config.fc1_stlink_serial.empty()) {
        return std::unexpected{std::string{"missing required key 'fc1_stlink_serial'"}};
    }
    if (config.fc2_stlink_serial.empty()) {
        return std::unexpected{std::string{"missing required key 'fc2_stlink_serial'"}};
    }
    return config;
}

/* Opens the file without enabling stream exceptions and includes its path in errors. */
std::expected<HilHardwareConfig, std::string>
load_hil_hardware_config(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input.is_open()) {
        return std::unexpected{"cannot open HIL hardware configuration '" + path.string() + "'"};
    }
    auto result = parse_hil_hardware_config(input);
    if (!result) {
        return std::unexpected{"HIL hardware configuration '" + path.string() +
                               "': " + result.error()};
    }
    return result;
}

}
