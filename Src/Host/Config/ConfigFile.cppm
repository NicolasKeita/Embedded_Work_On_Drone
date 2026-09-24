/*
Filename: Src/Host/Config/ConfigFile.cppm
Description: Host-only typed configuration reader and reproducibility snapshots.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ConfigFile;

import std;

export namespace sim::config {

struct ConfigEntry {
    std::string value{};
    std::uint64_t line = 0;
    bool consumed = false;
};

class Reader {
public:
    /* Creates an empty document with a diagnostic source label. */
    explicit Reader(std::string source);

    /* Adds a parsed assignment, rejecting duplicate keys. */
    void add(std::string key, std::string value, std::uint64_t line);

    /* Reads a finite number in the inclusive range, retaining the default if absent. */
    void number(std::string_view key, std::float64_t& destination,
                std::float64_t minimum, std::float64_t maximum);

    /* Reads an unsigned integer in the inclusive range, retaining the default if absent. */
    void unsigned_integer(std::string_view key, std::uint64_t& destination,
                          std::uint64_t minimum, std::uint64_t maximum);

    /* Reads true or false, retaining the default if absent. */
    void boolean(std::string_view key, bool& destination);

    /* Reads a nonempty text value, retaining the default if absent. */
    void text(std::string_view key, std::string& destination);

    /* Rejects unread keys and records all effective values for the run snapshot. */
    [[nodiscard]] std::expected<void, std::string> finish();

    /* Serializes the resolved values in stable key order. */
    [[nodiscard]] std::string resolved_text() const;

private:
    /* Returns and marks a supplied assignment, or null for an omitted value. */
    [[nodiscard]] ConfigEntry* take(std::string_view key);

    /* Keeps the first diagnostic with its source and key. */
    void fail(std::string_view key, std::string_view reason);

    std::string source_{};
    std::map<std::string, ConfigEntry, std::less<>> entries_{};
    std::map<std::string, std::string, std::less<>> resolved_{};
    std::string error_{};
};

/* Parses key=value text with hash comments and reports syntax or I/O errors. */
[[nodiscard]] std::expected<Reader, std::string>
parse_config_file(std::istream& input, std::string_view source);

/* Opens and parses a required configuration file before simulation starts. */
[[nodiscard]] std::expected<Reader, std::string>
load_config_file(const std::filesystem::path& path);

/* Writes every resolved document and the final runtime overrides before a run. */
[[nodiscard]] std::expected<void, std::string>
write_configuration_snapshot(const std::filesystem::path& path, std::string_view runtime_overrides);

}
