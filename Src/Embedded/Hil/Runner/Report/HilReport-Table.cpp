/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Table.cpp
Description: Shared report helpers of the HIL report : fixed telemetry table layout and
row writers, verdict wording, event suffixes and the single event line writer.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import FlightController;
import HealthMonitor;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;
import SafetyManager;

namespace sim::hil {

namespace {
    constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;
}

std::string_view yes_no(bool value) noexcept { return value ? "YES" : "NO"; }

void write_metric(std::ostream& out, std::float64_t value)
{
    out << std::fixed << std::setprecision(2) << value;
}

std::array<std::float64_t, 10> row_values(const HilSensorSample& s)
{
    return {s.time_s, s.x, s.y, s.z, s.vx, s.vy, s.vz, s.pitch_rad * kDegreesPerRadian,
            s.roll_rad * kDegreesPerRadian, s.measured_rpm};
}

void write_table_header(std::ostream& out)
{
    out << " ";
    for (std::size_t c = 0; c < 10; ++c) {
        out << std::setw(kReportWidths[c]) << kReportColumns[c] << "  ";
    }
    out << "\n";
}

void write_table_row(std::ostream& out, const HilSensorSample& s)
{
    const auto values = row_values(s);

    out << " ";
    for (std::size_t c = 0; c < 10; ++c) {
        out << std::setw(kReportWidths[c]);
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
        return !event.new_state.empty() ? event.new_state : event.detail;
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
