/*
Filename: Src/Embedded/Hil/HilRunner-Report.cpp
Description: Human-readable HIL mission report : configuration, the 1 Hz telemetry table
(sensor stream) interleaved with the structured event timeline, a ground-truth vs sensor
altitude comparison so the two streams stay explicitly distinguishable, communication
and timing statistics, and the mission result/verdict.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FlightController;
import HealthMonitor;
import HilEvents;
import HilConfig;
import HilRunnerContext;
import HilTelemetry;
import SafetyManager;

namespace sim::hil {

namespace {
    constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;

    const std::array<std::string_view, 10> kColumns{
        "t(s)", "x(m)", "y(m)", "z(m)", "vx(m/s)", "vy(m/s)", "vz(m/s)", "pitch(deg)", "roll(deg)", "rpm"};
    const std::array<int, 10> kWidths{6, 8, 8, 8, 8, 8, 8, 10, 9, 9};

    std::string_view yes_no(bool value) noexcept { return value ? "YES" : "NO"; }

    void write_metric(std::ostream& out, std::float64_t value) { out << std::fixed << std::setprecision(2) << value; }

    std::array<std::float64_t, 10> row_values(const HilSensorSample& s)
    {
        return {s.time_s, s.x, s.y, s.z, s.vx, s.vy, s.vz, s.pitch_rad * kDegreesPerRadian,
                s.roll_rad * kDegreesPerRadian, s.measured_rpm};
    }

    void write_table_header(std::ostream& out)
    {
        out << " ";
        for (std::size_t c = 0; c < 10; ++c) {
            out << std::setw(kWidths[c]) << kColumns[c] << "  ";
        }
        out << "\n";
    }

    void write_table_row(std::ostream& out, const HilSensorSample& s)
    {
        const auto values = row_values(s);
        out << " ";
        for (std::size_t c = 0; c < 10; ++c) {
            out << std::setw(kWidths[c]);
            write_metric(out, values[c]);
            out << "  ";
        }
        out << "\n";
    }

    std::string_view hil_verdict_reason(const HilResult& r)
    {
        if (!r.test_verdict) {
            return "behavior not compliant with scenario expectations";
        }
        if (r.final_state == sim::control::MissionState::ABORTED) {
            return "fault detected and SAFE_MODE engaged within the required limits";
        }
        if (r.final_safety_mode == sim::safety::SafetyMode::COMPENSATED) {
            return "degrading fault compensated, mission pursued";
        }
        return "nominal mission completed with no abnormal behavior";
    }

    std::string_view event_suffix(const HilEvent& event)
    {
        switch (event.type) {
        case HilEventType::MissionStateTransition:
        case HilEventType::SafetyStateTransition:
            return event.new_state;
        case HilEventType::MissionComplete:
            return "MISSION_COMPLETE";
        case HilEventType::MissionAborted:
            return "MISSION_ABORTED";
        case HilEventType::Fc1Failure:
            return "FC1_FAILURE";
        case HilEventType::HeartbeatTimeout:
            return "heartbeat timeout";
        case HilEventType::DeadlineMissed:
            return "deadline missed";
        case HilEventType::HilRunStart:
            return "HIL_RUN_START";
        case HilEventType::HilRunEnd:
            return "HIL_RUN_END";
        default:
            return event.detail;
        }
    }

    void write_event_line(std::ostream& out, const HilEvent& event)
    {
        out << "[" << event_category(event.type) << "] t = " << std::fixed << std::setprecision(2)
            << event.sim_time_s << " s -> " << event_suffix(event);
        if (event.has_duration) {
            out << " (lateness " << event.duration_us << " us)";
        }
        out << "\n";
    }
}

void HilRunner::writeReport(std::ostream& out, const HilRunOutput& output)
{
    const HilConfig& cfg = output.config;
    const HilResult& r = output.result;

    out << "\n============================================\n";
    out << cfg.scenario_id << " : " << (r.fault_expected ? "fault injection scenario" : "nominal station keeping")
        << "\n";
    out << "============================================\n\n";
    if (!r.fault_expected) {
        out << "Host-target closed-loop validation (NO physical STM32 present).\n\n";
    }

    out << "Configuration\n";
    out << "  Duration              : " << std::fixed << std::setprecision(1) << cfg.duration_s << " s\n";
    out << "  Control period        : " << std::setprecision(3) << cfg.dt_s << " s\n";
    out << "  Control frequency     : " << std::setprecision(0) << std::round(1.0 / cfg.dt_s) << " Hz\n";
    out << "  Random seed           : " << cfg.seed << "\n";
    out << "  Target altitude       : " << std::fixed << std::setprecision(1) << cfg.target.z << " m\n";
    out << "  Sensor noise stddev   : " << std::setprecision(3) << cfg.sensor_noise_stddev << " m\n";
    out << "  Real-time pacing      : " << yes_no(cfg.real_time_pacing) << " (" << deadline_policy_name(cfg.deadline_policy)
        << ")\n";
    out << "  FC target             : host emulator fc1_hil_host core (STM32 target is FUTURE)\n\n";

    out << "Mission (sensor stream : FC-observed; ground truth recorded separately)\n";
    write_table_header(out);

    const std::uint64_t report_steps = static_cast<std::uint64_t>(std::ceil(cfg.duration_s / cfg.report_period_s));
    std::vector<HilEvent> timeline;
    for (const HilEvent& event : output.events) {
        if (is_report_event(event.type)) {
            timeline.push_back(event);
        }
    }
    std::size_t event_idx = 0;
    for (std::uint64_t k = 0; k <= report_steps; ++k) {
        const std::float64_t target_time = static_cast<std::float64_t>(k) * cfg.report_period_s;
        std::ptrdiff_t best = -1;
        std::float64_t best_delta = 1.0e9;
        for (std::ptrdiff_t i = 0; i < static_cast<std::ptrdiff_t>(output.telemetry.size()); ++i) {
            const std::float64_t d = std::abs(output.telemetry[static_cast<std::size_t>(i)].time_s - target_time);
            if (d < best_delta) {
                best_delta = d;
                best = i;
            }
        }
        const std::float64_t boundary = (best >= 0) ? output.telemetry[static_cast<std::size_t>(best)].time_s : target_time;
        while (event_idx < timeline.size() && timeline[event_idx].sim_time_s <= boundary + 1.0e-6) {
            write_event_line(out, timeline[event_idx]);
            ++event_idx;
        }
        if (best >= 0) {
            write_table_row(out, output.telemetry[static_cast<std::size_t>(best)]);
            if (best < static_cast<std::ptrdiff_t>(output.ground_truth.size())) {
                const HilTruthSample& truth = output.ground_truth[static_cast<std::size_t>(best)];
                const HilSensorSample& sensor = output.telemetry[static_cast<std::size_t>(best)];
                out << "    truth z=" << std::fixed << std::setprecision(3) << truth.z
                    << " m  sensor z=" << sensor.z << " m\n";
            }
        }
    }
    while (event_idx < timeline.size()) {
        write_event_line(out, timeline[event_idx]);
        ++event_idx;
    }
    out << "\n";

    out << "Events\n";
    for (const HilEvent& event : timeline) {
        out << " " << std::fixed << std::setprecision(3) << event.sim_time_s << "  " << event_type_name(event.type);
        if (!event.detail.empty()) {
            out << " (" << event.detail << ")";
        }
        out << "\n";
    }
    out << "\n";

    out << "Timing\n";
    out << "  Steps executed        : " << r.timing.steps_executed << "\n";
    out << "  Deadline misses       : " << r.timing.deadline_misses << "\n";
    out << "  Max step duration     : " << r.timing.max_step_us << " us\n";
    out << "  Mean step duration    : " << static_cast<std::int64_t>(r.timing.mean_step_us) << " us\n";
    out << "  Max round trip        : " << r.timing.max_round_trip_us << " us\n";
    out << "  Max lateness          : " << r.timing.max_lateness_us << " us\n\n";

    out << "Communication statistics\n";
    out << "  Messages sent         : " << r.comms.messages_sent << "\n";
    out << "  Messages received     : " << r.comms.messages_received << "\n";
    out << "  Messages dropped      : " << r.comms.messages_dropped << "\n";
    out << "  Sequence errors       : " << r.comms.sequence_errors << "\n";
    out << "  Stale packets         : " << r.comms.stale_packets << "\n";
    out << "  Timeouts              : " << r.comms.timeouts << "\n";
    out << "  Latency min/mean/max  : " << r.comms.latency_min_us << " / "
        << static_cast<std::int64_t>(r.comms.latency_mean_us) << " / " << r.comms.latency_max_us << " us\n\n";

    out << "Mission result\n";
    out << "  Mission success       : " << yes_no(r.mission_success) << "\n";
    out << "  Final mission state   : " << sim::control::mission_state_name(r.final_state) << "\n";
    out << "  Final safety mode     : " << sim::safety::safety_mode_name(r.final_safety_mode) << "\n";
    out << "  Final health          : " << sim::safety::health_state_name(r.final_health) << "\n";
    out << "  Fault detected        : " << yes_no(r.fault_detected) << "\n";
    out << "  Detection latency     : " << std::fixed << std::setprecision(4) << r.detection_latency << " s\n";
    out << "  Test verdict          : " << (r.test_verdict ? "PASS" : "FAIL") << "\n";
    out << "  > " << hil_verdict_reason(r) << "\n";
}

}
