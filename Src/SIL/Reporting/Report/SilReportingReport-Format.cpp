/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Format.cpp
Description: Allocation-free shared formatting helpers for the Markdown and CSV report sections.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import SilFaultScenario;

namespace sim::sil {

namespace {
constexpr std::streamsize kSecondsPrecision = 3;
constexpr std::streamsize kMetricPrecision = 2;

/*
Writes a std::float64_t into the stream with the requested fixed precision,
restoring the stream flags afterwards (no dynamic allocation).
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
Streams the CSV-escaped text: raw when no separator is present, otherwise
wrapped in quotes with internal quotes doubled.
*/
void write_csv_escaped(std::ostream& out, std::string_view text)
{
    if (text.find_first_of(";\"\n") == std::string_view::npos) {
        out << text;
        return;
    }
    out << '\"';
    for (const char item : text) {
        if (item == '\"') {
            out << "\"\"";
        }
        else {
            out << item;
        }
    }
    out << '\"';
}

std::string_view yes_no(bool value) noexcept
{
    return value ? "yes" : "no";
}

/*
Streams the labelled numeric parameter of one fault event: the label and unit
follow the value kind recorded at injection time, and the actuator efficiency
also shows the resulting capacity loss. Nothing is streamed when the event
carries no typed parameter.
*/
void write_fault_value(std::ostream& out, const SilEvent& event)
{
    switch (event.value_kind) {
    case FaultValueKind::Efficiency: {
        const std::int64_t capacity_loss_percent = static_cast<std::int64_t>((1.0 - event.value) * 100.0 + 0.5);

        out << "efficiency=";
        write_metric(out, event.value);
        if (capacity_loss_percent > 0) {
            out << " (-" << capacity_loss_percent << "% capacity)";
        }
        break;
    }
    case FaultValueKind::LossProbability:
        out << "loss_probability=";
        write_metric(out, event.value);
        break;
    case FaultValueKind::Altitude:
        out << "value=";
        if (event.has_value) {
            write_metric(out, event.value);
            out << " m";
        }
        else {
            out << "NaN m";
        }
        break;
    case FaultValueKind::AltitudeNoise:
        out << "amplitude=";
        write_metric(out, event.value);
        out << " m";
        break;
    case FaultValueKind::None:
        break;
    }
}

}
