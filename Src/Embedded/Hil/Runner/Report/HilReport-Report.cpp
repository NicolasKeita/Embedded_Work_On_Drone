/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Report.cpp
Description: Full HIL mission report : the 1 Hz telemetry table interleaved with the
structured event timeline, the ground-truth vs sensor altitude comparison and the event
list, closed by the post-run summary.

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

namespace {
    std::vector<HilEvent> report_timeline(const HilRunOutput& output)
    {
        std::vector<HilEvent> timeline;

        for (const HilEvent& event : output.events) {
            if (is_report_event(event.type)) {
                timeline.push_back(event);
            }
        }
        return timeline;
    }

    std::ptrdiff_t nearest_sample(const HilRunOutput& output, std::float64_t target_time)
    {
        std::ptrdiff_t best = -1;
        std::float64_t best_delta = 1.0e9;

        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(output.telemetry.size()); ++i) {
            const std::float64_t d = std::abs(output.telemetry[static_cast<std::size_t>(i)].time_s - target_time);
            if (d < best_delta) {
                best_delta = d;
                best = i;
            }
        }
        return best;
    }

    void write_timeline_row(std::ostream& out, const HilRunOutput& output, std::ptrdiff_t best)
    {
        if (best < 0) {
            return;
        }
        write_table_row(out, output.telemetry[static_cast<std::size_t>(best)]);
        if (best < static_cast<std::ptrdiff_t>(output.ground_truth.size())) {
            const HilTruthSample& truth = output.ground_truth[static_cast<std::size_t>(best)];
            const HilSensorSample& sensor = output.telemetry[static_cast<std::size_t>(best)];
            out << "    truth z=" << std::fixed << std::setprecision(3) << truth.z
                << " m  sensor z=" << sensor.z << " m\n";
        }
    }

    void write_mission_timeline(std::ostream&                out,
                                const HilRunOutput&          output,
                                const std::vector<HilEvent>& timeline)
    {
        const HilConfig&    cfg = output.config;
        const std::uint64_t report_steps = static_cast<std::uint64_t>(std::ceil(cfg.duration_s / cfg.report_period_s));
        std::size_t         event_idx = 0;

        for (std::uint64_t k = 0; k <= report_steps; ++k) {
            const std::float64_t target_time = static_cast<std::float64_t>(k) * cfg.report_period_s;
            const std::ptrdiff_t best = nearest_sample(output, target_time);
            const std::float64_t boundary = (best >= 0) ? output.telemetry[static_cast<std::size_t>(best)].time_s
                                                        : target_time;
            while (event_idx < timeline.size() && timeline[event_idx].sim_time_s <= boundary + 1.0e-6) {
                write_event_line(out, timeline[event_idx]);
                ++event_idx;
            }
            write_timeline_row(out, output, best);
        }
        while (event_idx < timeline.size()) {
            write_event_line(out, timeline[event_idx]);
            ++event_idx;
        }
        out << "\n";
    }

    void write_event_list(std::ostream& out, const std::vector<HilEvent>& timeline)
    {
        out << "Events\n";
        for (const HilEvent& event : timeline) {
            out << " " << std::fixed << std::setprecision(3) << event.sim_time_s << "  "
                << event_type_name(event.type);
            if (!event.detail.empty()) {
                out << " (" << event.detail << ")";
            }
            out << "\n";
        }
    }
}

void write_report(std::ostream& out, const HilRunOutput& output)
{
    write_header(out, output.config, output.result.fault_expected);
    const std::vector<HilEvent> timeline = report_timeline(output);

    write_mission_timeline(out, output, timeline);
    write_event_list(out, timeline);
    write_summary(out, output);
}

}
