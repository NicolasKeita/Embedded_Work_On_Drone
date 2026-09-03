/*
Filename: Src/SIL/Reporting/SilReporting-Format.cpp
Description: Allocation-free formatting and file writing implementations for SIL reports.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

namespace sim::sil {

namespace {
constexpr std::streamsize kSecondsPrecision = 3;
constexpr std::streamsize kMetricPrecision = 2;

/*
Writes a std::float64_t into the stream with the requested fixed precision, restoring
the stream flags afterwards (no dynamic allocation).
*/
void write_number(std::ostream& out, std::float64_t value, std::streamsize precision)
{
    const std::ios_base::fmtflags flags = out.flags();

    out << std::fixed << std::setprecision(precision) << value;
    out.flags(flags);
}
}

/*
Writes a timestamp value with three decimals into the stream.
*/
void write_seconds(std::ostream& out, std::float64_t value)
{
    write_number(out, value, kSecondsPrecision);
}

/*
Writes a metric value with two decimals into the stream.
*/
void write_metric(std::ostream& out, std::float64_t value)
{
    write_number(out, value, kMetricPrecision);
}

/*
Streams the JSON-escaped text (std::float64_t quotes and backslashes) without building
any intermediate string.
*/
void write_json_escaped(std::ostream& out, std::string_view text)
{
    for (const char item : text) {
        if (item == '"') {
            out << "\\\"";
        }
        else if (item == '\\') {
            out << "\\\\";
        }
        else {
            out << item;
        }
    }
}

/*
Streams the CSV-escaped text: raw when no separator is present, otherwise
wrapped in std::float64_t quotes with internal quotes doubled.
*/
void write_csv_escaped(std::ostream& out, std::string_view text)
{
    if (text.find_first_of(";\"\n") == std::string_view::npos) {
        out << text;
        return;
    }
    out << '"';
    for (const char item : text) {
        if (item == '"') {
            out << "\"\"";
        }
        else {
            out << item;
        }
    }
    out << '"';
}

std::string_view yes_no(bool value) noexcept
{
    return value ? "oui" : "non";
}

}
