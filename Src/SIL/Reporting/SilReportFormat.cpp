/*
Filename: Src/SIL/Reporting/SilReportFormat.cpp
Description: Formatting and file writing implementations for SIL reports.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportFormat;

import std;

namespace sim::sil {

std::string format_seconds(double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(3) << value;
    return stream.str();
}

std::string format_metric(double value)
{
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(2) << value;
    return stream.str();
}

std::string json_escape(std::string_view text)
{
    std::string escaped;
    escaped.reserve(text.size());
    for (const char item : text) {
        if (item == '"') {
            escaped += "\\\"";
        }
        else if (item == '\\') {
            escaped += "\\\\";
        }
        else {
            escaped += item;
        }
    }
    return escaped;
}

std::string csv_escape(std::string_view text)
{
    if (text.find_first_of(";\"\n") == std::string_view::npos) {
        return std::string{text};
    }
    std::string escaped{"\""};
    for (const char item : text) {
        if (item == '"') {
            escaped += "\"\"";
        }
        else {
            escaped += item;
        }
    }
    escaped += '"';
    return escaped;
}

std::string_view yes_no(bool value)
{
    return value ? "oui" : "non";
}

void write_file(const std::filesystem::path& path, std::string_view content)
{
    std::ofstream file{path};
    file << content;
}

}
