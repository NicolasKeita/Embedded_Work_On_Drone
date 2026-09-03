/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Table.cpp
Description: Aligned telemetry Markdown table writer (fixed-width columns, header and rows).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import SilTelemetry;

namespace sim::sil {

namespace {

constexpr std::float64_t kTimeEpsilon      = 1.0e-9;
constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;
constexpr std::size_t    kColumnCount      = 10;

const std::array<std::string_view, kColumnCount> kColumnHeaders{
    "t(s)", "x(m)", "y(m)", "z(m)", "vx(m/s)", "vy(m/s)", "vz(m/s)", "pitch(deg)", "roll(deg)", "rpm"};

const std::array<std::size_t, kColumnCount> kColumnWidths{6, 6, 6, 8, 7, 7, 7, 10, 9, 8};

/* Collects the ten displayed values of one sample (angles converted to degrees) in column order. */
std::array<std::float64_t, kColumnCount> row_values(const TelemetrySample& sample)
{
    return {sample.time,
            sample.x,
            sample.y,
            sample.altitude_m,
            sample.vx,
            sample.vy,
            sample.vz,
            sample.pitch_rad * kDegreesPerRadian,
            sample.roll_rad * kDegreesPerRadian,
            sample.actual_rpm};
}

/*
Streams the aligned header row and its dash separator (at least three dashes
per column, so the table stays valid Markdown).
*/
void write_telemetry_header(std::ostream& out)
{
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        out << "| " << std::setw(static_cast<int>(kColumnWidths[column])) << kColumnHeaders[column] << ' ';
    }
    out << "|\n";
    for (std::size_t column = 0; column < kColumnCount; ++column) {
        out << '|';
        for (std::size_t dash = 0; dash < kColumnWidths[column] + 2; ++dash) {
            out.put('-');
        }
    }
    out << "|\n";
}

/* Streams one aligned telemetry row, every value padded to its fixed column width. */
void write_telemetry_row(std::ostream& out, const TelemetrySample& sample)
{
    const std::array<std::float64_t, kColumnCount> values = row_values(sample);

    for (std::size_t column = 0; column < kColumnCount; ++column) {
        out << "| " << std::setw(static_cast<int>(kColumnWidths[column]));
        write_metric(out, values[column]);
        out << ' ';
    }
    out << "|\n";
}

}

/*
Writes one full telemetry table over the selected time window: the first sample
at or after start_after opens the cadence, then one row every interval_s until
stop_before. Fixed-width columns keep the raw text aligned when the report is
dumped to a terminal, while remaining valid Markdown.
*/
void write_telemetry_window(std::ostream&                    out,
                            std::span<const TelemetrySample> samples,
                            std::float64_t                   start_after,
                            std::float64_t                   stop_before,
                            std::float64_t                   interval_s)
{
    std::float64_t next_time = start_after;

    out << std::right;
    write_telemetry_header(out);
    for (const TelemetrySample& sample : samples) {
        if (sample.time + kTimeEpsilon < next_time || sample.time >= stop_before - kTimeEpsilon) {
            continue;
        }
        write_telemetry_row(out, sample);
        next_time = sample.time + interval_s;
    }
}

}
