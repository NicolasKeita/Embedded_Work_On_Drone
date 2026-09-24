/*
Filename: Src/Host/Config/ConfigFile.cpp
Description: Host startup configuration parsing, validation and resolved snapshots.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ConfigFile;

import std;

namespace sim::config {
namespace {

std::map<std::string, std::string> resolved_documents{};

/* Trims the ASCII whitespace accepted around configuration assignments. */
[[nodiscard]] std::string_view trim(std::string_view value)
{
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string_view::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

/* Restricts keys to portable dotted identifiers rather than silently accepting typos. */
[[nodiscard]] bool valid_key(std::string_view key)
{
    return !key.empty() && key.find_first_not_of(
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.-") == std::string_view::npos;
}

/* Formats a source line diagnostic without enabling stream exceptions. */
[[nodiscard]] std::string syntax_error(std::string_view source, std::uint64_t line,
                                      std::string_view reason)
{
    return std::format("{}:{}: {}", source, line, reason);
}

}

/* Creates an empty document with a diagnostic source label. */
Reader::Reader(std::string source) : source_{std::move(source)}
{
}

/* Keeps the first diagnostic with its source and key. */
void Reader::fail(std::string_view key, std::string_view reason)
{
    if (error_.empty()) {
        const auto entry = entries_.find(key);
        const std::uint64_t line = entry == entries_.end() ? 0 : entry->second.line;
        error_ = std::format("{}:{}: {}: {}", source_, line, key, reason);
    }
}

/* Adds a parsed assignment, rejecting duplicate keys. */
void Reader::add(std::string key, std::string value, std::uint64_t line)
{
    if (entries_.contains(key)) {
        fail(key, "duplicate key");
        return;
    }
    entries_.emplace(std::move(key), ConfigEntry{std::move(value), line, false});
}

/* Returns and marks a supplied assignment, or null for an omitted value. */
ConfigEntry* Reader::take(std::string_view key)
{
    const auto entry = entries_.find(key);
    if (entry == entries_.end()) {
        return nullptr;
    }
    entry->second.consumed = true;
    return &entry->second;
}

/* Reads and range-checks one floating-point parameter before recording its effective value. */
void Reader::number(std::string_view key, std::float64_t& destination,
                    std::float64_t minimum, std::float64_t maximum)
{
    if (const ConfigEntry* entry = take(key)) {
        std::float64_t parsed = 0.0;
        const auto result = std::from_chars(entry->value.data(),
                                            entry->value.data() + entry->value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != entry->value.data() + entry->value.size()
            || !std::isfinite(parsed) || parsed < minimum || parsed > maximum) {
            fail(key, std::format("expected a finite number in [{}, {}]", minimum, maximum));
            return;
        }
        destination = parsed;
    }
    if (!std::isfinite(destination) || destination < minimum || destination > maximum) {
        fail(key, "default value is outside the permitted range");
        return;
    }
    resolved_[std::string{key}] = std::format("{:.17g}", destination);
}

/* Reads and range-checks one unsigned integer without permitting signs or fractional values. */
void Reader::unsigned_integer(std::string_view key, std::uint64_t& destination,
                              std::uint64_t minimum, std::uint64_t maximum)
{
    if (const ConfigEntry* entry = take(key)) {
        std::uint64_t parsed = 0;
        const auto result = std::from_chars(entry->value.data(),
                                            entry->value.data() + entry->value.size(), parsed);
        if (result.ec != std::errc{} || result.ptr != entry->value.data() + entry->value.size()
            || parsed < minimum || parsed > maximum) {
            fail(key, std::format("expected an integer in [{}, {}]", minimum, maximum));
            return;
        }
        destination = parsed;
    }
    if (destination < minimum || destination > maximum) {
        fail(key, "default value is outside the permitted range");
        return;
    }
    resolved_[std::string{key}] = std::to_string(destination);
}

/* Reads a strict boolean and records the value used by the run. */
void Reader::boolean(std::string_view key, bool& destination)
{
    if (const ConfigEntry* entry = take(key)) {
        if (entry->value != "true" && entry->value != "false") {
            fail(key, "expected true or false");
            return;
        }
        destination = entry->value == "true";
    }
    resolved_[std::string{key}] = destination ? "true" : "false";
}

/* Reads a nonempty string and records the value used by the run. */
void Reader::text(std::string_view key, std::string& destination)
{
    if (const ConfigEntry* entry = take(key)) {
        if (entry->value.empty()) {
            fail(key, "expected a nonempty value");
            return;
        }
        destination = entry->value;
    }
    resolved_[std::string{key}] = destination;
}

/* Serializes resolved defaults and overrides in stable key order. */
std::string Reader::resolved_text() const
{
    std::string result{};
    for (const auto& [key, value] : resolved_) {
        result += key + " = " + value + "\n";
    }
    return result;
}

/* Rejects unrecognized assignments and registers only successfully validated documents. */
std::expected<void, std::string> Reader::finish()
{
    for (const auto& [key, entry] : entries_) {
        if (!entry.consumed) {
            fail(key, "unknown key");
        }
    }
    if (!error_.empty()) {
        return std::unexpected(error_);
    }
    resolved_documents[source_] = resolved_text();
    return {};
}

/* Parses bounded key=value lines with hash comments and CRLF support. */
std::expected<Reader, std::string> parse_config_file(std::istream& input, std::string_view source)
{
    Reader reader{std::string{source}};
    std::string line{};
    std::uint64_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        if (line.size() > 4096 || line_number > 4096) {
            return std::unexpected(syntax_error(source, line_number, "configuration exceeds size limits"));
        }
        const std::string_view raw{line};
        const auto entry = trim(raw.substr(0, raw.find('#')));
        if (entry.empty()) {
            continue;
        }
        const auto separator = entry.find('=');
        if (separator == std::string_view::npos) {
            return std::unexpected(syntax_error(source, line_number, "expected key=value"));
        }
        const auto key = trim(entry.substr(0, separator));
        const auto value = trim(entry.substr(separator + 1));
        if (!valid_key(key) || value.empty()) {
            return std::unexpected(syntax_error(source, line_number, "invalid key or empty value"));
        }
        reader.add(std::string{key}, std::string{value}, line_number);
    }
    if (input.bad() || !input.eof()) {
        return std::unexpected(syntax_error(source, line_number + 1, "unable to read configuration"));
    }
    return reader;
}

/* Loads a required host configuration file without throwing I/O exceptions. */
std::expected<Reader, std::string> load_config_file(const std::filesystem::path& path)
{
    std::ifstream input{path};
    if (!input.is_open()) {
        return std::unexpected(std::format("cannot open configuration '{}'", path.string()));
    }
    return parse_config_file(input, path.string());
}

/* Saves the complete resolved file inputs plus the effective runtime and CLI overrides. */
std::expected<void, std::string> write_configuration_snapshot(
    const std::filesystem::path& path, std::string_view runtime_overrides)
{
    std::error_code error{};
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
    }
    if (error) {
        return std::unexpected(std::format("cannot create snapshot directory: {}", error.message()));
    }
    std::ofstream output{path};
    if (!output.is_open()) {
        return std::unexpected(std::format("cannot write configuration snapshot '{}'", path.string()));
    }
    output << "# Resolved host configuration; runtime overrides below take precedence.\n";
    for (const auto& [source, values] : resolved_documents) {
        output << "\n[source " << source << "]\n" << values;
    }
    output << "\n[runtime]\n" << runtime_overrides << '\n';
    output.flush();
    if (!output) {
        return std::unexpected(std::format("cannot finish configuration snapshot '{}'", path.string()));
    }
    return {};
}

}
