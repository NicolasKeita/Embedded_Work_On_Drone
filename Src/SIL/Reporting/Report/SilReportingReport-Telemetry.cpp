/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Telemetry.cpp
Description: Periodic telemetry and post-fault sections of the Markdown report.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTelemetry;
import SilTypes;

namespace sim::sil {

namespace {

constexpr std::float64_t kTimeEpsilon      = 1.0e-9;
constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;

/* Streams the header row of a telemetry table. */
void write_telemetry_header(std::ostream& out)
{
    out << "| t(s) | x(m) | y(m) | z(m) | vx(m/s) | vy(m/s) | vz(m/s) | pitch(deg) | roll(deg) | rpm |\n";
    out << "|---|---|---|---|---|---|---|---|---|---|\n";
}

/*
Streams downsampled rows: the first sample at or after start_after opens the
cadence, then one row every interval_s until stop_before.
*/
void write_telemetry_rows(std::ostream&                    out,
                          std::span<const TelemetrySample> samples,
                          std::float64_t                   start_after,
                          std::float64_t                   stop_before,
                          std::float64_t                   interval_s)
{
    std::float64_t next_time = start_after;

    for (const TelemetrySample& sample : samples) {
        if (sample.time + kTimeEpsilon < next_time || sample.time >= stop_before - kTimeEpsilon) {
            continue;
        }
        out << "| ";
        write_metric(out, sample.time);
        out << " | ";
        write_metric(out, sample.x);
        out << " | ";
        write_metric(out, sample.y);
        out << " | ";
        write_metric(out, sample.altitude_m);
        out << " | ";
        write_metric(out, sample.vx);
        out << " | ";
        write_metric(out, sample.vy);
        out << " | ";
        write_metric(out, sample.vz);
        out << " | ";
        write_metric(out, sample.pitch_rad * kDegreesPerRadian);
        out << " | ";
        write_metric(out, sample.roll_rad * kDegreesPerRadian);
        out << " | ";
        write_metric(out, sample.actual_rpm);
        out << " |\n";
        next_time = sample.time + interval_s;
    }
}

}

/*
Writes the full periodic mission telemetry table (used when no fault splits the
mission into pre/post windows).
*/
void write_telemetry_table(std::ostream&                    out,
                           std::span<const TelemetrySample> samples,
                           std::float64_t                   interval_s)
{
    write_telemetry_header(out);
    write_telemetry_rows(out, samples, 0.0, std::numeric_limits<std::float64_t>::max(), interval_s);
}

/*
Appends the telemetry/event/post-fault sections of one detailed scenario
section: mission telemetry, important events, then the post-fault window.
*/
void write_scenario_telemetry_sections(std::ostream&         out,
                                       const ScenarioRecord& record,
                                       std::float64_t        telemetry_report_interval_s)
{
    const std::float64_t fault_time = record.result.fault_injected_time;

        out << "\n#### Telemetrie mission\n\n";
    write_telemetry_header(out);
    const std::float64_t pre_fault_stop = fault_time < 0.0 ? std::numeric_limits<std::float64_t>::max() : fault_time;
    write_telemetry_rows(out, record.telemetry, 0.0, pre_fault_stop, telemetry_report_interval_s);

    out << "\n#### Evenements\n\n";
    write_event_table(out, record.events);
    if (fault_time < 0.0) {
        out << "\n";
        return;
    }
    out << "\n#### Telemetrie apres faulte\n\n";
    write_telemetry_header(out);
    write_telemetry_rows(out, record.telemetry, fault_time, std::numeric_limits<std::float64_t>::max(),
                         telemetry_report_interval_s);
    out << "\n";
}

}
