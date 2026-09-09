/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Live.cpp
Description: Live streaming of the HIL run : event lines, telemetry rows and Twin snapshots.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;
import TwinWebSocketPublisher;

namespace sim::hil {

namespace {
    /* Process-local non-blocking viewer publisher shared by the live stream. */
    TwinWebSocketPublisher twin_publisher{};

    /* Maps a HIL event severity to the viewer event-level vocabulary. */
    std::string_view viewer_event_level(HilEventSeverity severity)
    {
        if (severity == HilEventSeverity::Error) {
            return "critical";
        }
        return severity == HilEventSeverity::Warning ? "warn" : "info";
    }
}

/* Writes recent high-level HIL events into the current TwinSnapshot. */
void write_recent_events(std::ostream& out, std::span<const HilEvent> events)
{
    const std::size_t first = events.size() > 12 ? events.size() - 12 : 0;

    out << '[';
    bool wrote_event = false;
    for (std::size_t index = first; index < events.size(); ++index) {
        const HilEvent& event = events[index];
        if (!is_report_event(event.type)) {
            continue;
        }
        if (wrote_event) {
            out << ',';
        }
        out << "{\"time_s\":" << event.sim_time_s << ",\"type\":";
        write_json_string(out, event_category(event.type));
        out << ",\"message\":";
        const std::string_view message = event.detail.empty() ? event.reason : event.detail;
        write_json_string(out, message);
        out << ",\"level\":";
        write_json_string(out, viewer_event_level(event.severity));
        out << '}';
        wrote_event = true;
    }
    out << ']';
}

/*
Streams the run live to ctx.live_out (no-op when no stream is registered): drains the
trace events recorded since the previous call and prints the report-event lines, then
one telemetry row per report-period boundary crossed. Everything is flushed immediately.
*/
void stream_live_output(HilRunContext& ctx)
{
    if (ctx.live_out == nullptr) {
        return;
    }
    std::ostream& out = *ctx.live_out;
    const HilConfig& cfg = ctx.config;

    const std::span<const HilEvent> events = ctx.trace.events();
    while (ctx.live_event_cursor < events.size()) {
        const HilEvent& event = events[ctx.live_event_cursor];
        ++ctx.live_event_cursor;
        if (is_report_event(event.type)) {
            write_event_line(out, event);
        }
    }

    const std::uint64_t report_steps = static_cast<std::uint64_t>(std::ceil(cfg.duration_s / cfg.report_period_s));
    while (ctx.live_report_index <= report_steps
           && ctx.time + 1.0e-9 >= static_cast<std::float64_t>(ctx.live_report_index) * cfg.report_period_s) {
        ++ctx.live_report_index;
        if (ctx.telemetry_recorder.samples.empty()) {
            continue;
        }
        const HilSensorSample& sensor = ctx.telemetry_recorder.samples.back();
        write_table_row(out, sensor);
    }
    if (ctx.live_telemetry_cursor < ctx.telemetry_recorder.samples.size()) {
        ctx.live_telemetry_cursor = ctx.telemetry_recorder.samples.size();
        std::ostringstream snapshot;
        write_twin_snapshot(snapshot, ctx, ctx.telemetry_recorder.samples.back());
        const std::string payload = snapshot.str();
        twin_publisher.publish(payload);
    }
    out << std::flush;
}

}
