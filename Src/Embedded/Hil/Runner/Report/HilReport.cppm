/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport.cppm
Description: Human-readable HIL mission reporting : run header, the 1 Hz telemetry table
(sensor stream) interleaved with the structured event timeline, the ground-truth vs
sensor altitude comparison, communication/timing statistics, the mission result/verdict
and the live streaming variant emitted while the run executes.
Exports:
    write_header(),
    write_report(),
    write_summary(),
    stream_live_output()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilReport;

import std;

import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;

export namespace sim::hil {

/*
Writes the run header : scenario banner, host/target note, configuration block and the
telemetry table column header. Called before the run so the configuration is visible in
real time while the mission is executing.
*/
void write_header(std::ostream& out, const HilConfig& config, bool fault_expected);

/*
Writes the human-readable HIL report (configuration, 1 Hz telemetry interleaved with the
event timeline, communication/timing statistics, mission result and verdict).
*/
void write_report(std::ostream& out, const HilRunOutput& output);

/*
Writes the post-run summary : timing, communication statistics, mission result and
verdict. Only available once the run has completed.
*/
void write_summary(std::ostream& out, const HilRunOutput& output);

/*
Streams the run live to ctx.live_out (no-op when no stream is registered): drains the
trace events recorded since the previous call and prints the report-event lines, then
one telemetry row per report-period boundary crossed. Everything is flushed immediately.
*/
void stream_live_output(HilRunContext& ctx);

}

namespace sim::hil {

/*
Internal helpers shared across the HilReport-*.cpp translation units (module-internal,
not exported to importers): the fixed telemetry table layout, the row/line writers and
the verdict wording.
*/
inline constexpr std::array<std::string_view, 10> kReportColumns{
    "t(s)", "x(m)", "y(m)", "z(m)", "vx(m/s)", "vy(m/s)", "vz(m/s)", "pitch(deg)", "roll(deg)", "rpm"};
inline constexpr std::array<int, 10> kReportWidths{6, 8, 8, 8, 8, 8, 8, 10, 9, 9};

std::string_view yes_no(bool value) noexcept;
void write_metric(std::ostream& out, std::float64_t value);
std::array<std::float64_t, 10> row_values(const HilSensorSample& sample);
void write_table_header(std::ostream& out);
void write_table_row(std::ostream& out, const HilSensorSample& sample);
std::string_view hil_verdict_reason(const HilResult& result);
std::string_view event_suffix(const HilEvent& event);
void write_event_line(std::ostream& out, const HilEvent& event);

}