/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Live.cpp
Description: Live streaming of the HIL run : drains the trace events recorded since the
previous call and prints one telemetry row (with the truth-vs-sensor altitude line) per
report-period boundary crossed, flushed immediately.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;

namespace sim::hil {

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

    const std::uint64_t report_steps =
        static_cast<std::uint64_t>(std::ceil(cfg.duration_s / cfg.report_period_s));
    while (ctx.live_report_index <= report_steps
           && ctx.time + 1.0e-9 >= static_cast<std::float64_t>(ctx.live_report_index) * cfg.report_period_s) {
        ++ctx.live_report_index;
        if (ctx.telemetry_recorder.samples.empty()) {
            continue;
        }
        const HilSensorSample& sensor = ctx.telemetry_recorder.samples.back();
        write_table_row(out, sensor);
        if (!ctx.telemetry_recorder.truth_samples.empty()) {
            const HilTruthSample& truth = ctx.telemetry_recorder.truth_samples.back();
            out << "    truth z=" << std::fixed << std::setprecision(3) << truth.z
                << " m  sensor z=" << sensor.z << " m\n";
        }
    }
    out << std::flush;
}

}