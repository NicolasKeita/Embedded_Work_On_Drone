/*
Filename: Src/Embedded/Hil/Runner/Report/HilReport-Live.cpp
Description: Live streaming of the HIL run : drains the trace events recorded since the
previous call and prints one telemetry row per report-period boundary crossed, flushed
immediately.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilReport;

import std;

import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilTelemetry;
import FlightControllerTypes;
import HealthMonitor;
import SafetyManager;
import SilFaultScenario;
import TwinWebSocketPublisher;

namespace sim::hil {

namespace {
    /* Returns the process-local non-blocking viewer publisher. */
    TwinWebSocketPublisher& twin_publisher()
    {
        static TwinWebSocketPublisher publisher{};
        return publisher;
    }

    /* Writes one JSON string with the escaping required by the visualization stream. */
    void write_json_string(std::ostream& out, std::string_view value)
    {
        out << '"';
        for (const char character : value) {
            if (character == '"' || character == '\\') {
                out << '\\';
            }
            out << character;
        }
        out << '"';
    }

    /* Maps a HIL event severity to the viewer event-level vocabulary. */
    std::string_view viewer_event_level(HilEventSeverity severity)
    {
        if (severity == HilEventSeverity::Error) {
            return "critical";
        }
        if (severity == HilEventSeverity::Warning) {
            return "warn";
        }
        return "info";
    }

    /* Returns the logical FC1 component affected by the active injected failure. */
    std::string_view affected_component(const HilRunContext& ctx)
    {
        using sim::sil::FailureMode;

        if (!ctx.fault_active) {
            return {};
        }
        if (ctx.last_failure_mode == FailureMode::INVALID_SENSOR_DATA) {
            return "sensors";
        }
        if (ctx.last_failure_mode == FailureMode::ACTUATOR_DEGRADED) {
            return "actuators";
        }
        if (ctx.last_failure_mode == FailureMode::FC_COMMUNICATION_LOSS
            || ctx.last_failure_mode == FailureMode::COMMUNICATION_DEGRADED
            || ctx.last_failure_mode == FailureMode::FC1_UNAVAILABLE) {
            return "transport";
        }
        return {};
    }

    /* Writes the stable logical component map consumed by the procedural FC view. */
    void write_components(std::ostream& out, std::string_view affected, bool failed)
    {
        constexpr std::array<std::string_view, 7> names{
            "mcu", "transport", "sensors", "control", "actuators", "supervision", "safety"};

        out << '{';
        for (std::size_t index = 0; index < names.size(); ++index) {
            if (index > 0) {
                out << ',';
            }
            write_json_string(out, names[index]);
            out << ':';
            const bool affected_now = names[index] == affected;
            write_json_string(out, affected_now ? (failed ? "FAILED" : "DEGRADED") : "HEALTHY");
        }
        out << '}';
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

    /* Serializes the newest structured HIL sample as a browser TwinSnapshot line. */
    void write_twin_snapshot(std::ostream& out, const HilRunContext& ctx, const HilSensorSample& sample)
    {
        const std::string_view affected = affected_component(ctx);
        const bool failed = ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE;
        const std::float64_t airspeed = std::hypot(sample.vx, sample.vy, sample.vz);

        out << std::setprecision(8) << "{\"time_s\":" << sample.time_s;
        out << ",\"aircraft\":{\"x_m\":" << sample.x << ",\"y_m\":" << sample.y
            << ",\"z_m\":" << sample.z << ",\"altitude_m\":" << sample.z
            << ",\"pitch_rad\":" << sample.pitch_rad << ",\"roll_rad\":" << sample.roll_rad
            << ",\"airspeed_ms\":" << airspeed << '}';
        out << ",\"target\":{\"x_m\":" << sample.target_x << ",\"y_m\":" << sample.target_y
            << ",\"altitude_m\":" << sample.target_z << '}';
        out << ",\"actuators\":{\"rotor_rpm\":" << sample.measured_rpm
            << ",\"left_servo_deg\":" << sample.left_servo_deg
            << ",\"right_servo_deg\":" << sample.right_servo_deg << '}';
        out << ",\"mission\":";
        write_json_string(out, sim::control::mission_state_name(
                                   static_cast<sim::control::MissionState>(sample.mission_state)));
        out << ",\"health\":";
        write_json_string(out, sim::safety::health_state_name(ctx.result.final_health));
        out << ",\"safety_mode\":";
        write_json_string(out, sim::safety::safety_mode_name(ctx.safety.mode()));
        out << ",\"active_fault\":";
        if (ctx.fault_active) {
            write_json_string(out, sim::sil::failure_mode_name(ctx.last_failure_mode));
        }
        else {
            out << "null";
        }
        out << ",\"fc1\":{\"status\":";
        write_json_string(out, ctx.fc1_was_alive ? "ONLINE" : "OFFLINE");
        out << ",\"components\":";
        write_components(out, affected, failed);
        out << "},\"fc2\":{\"status\":\"ONLINE\",\"components\":";
        write_components(out, {}, false);
        out << "},\"hil\":{\"loop_hz\":" << (1.0 / ctx.config.dt_s)
            << ",\"deadline_misses\":" << ctx.timing.deadline_misses << "},\"events\":";
        write_recent_events(out, ctx.trace.events());
        out << '}';
    }
}

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
        twin_publisher().publish(payload);
    }
    out << std::flush;
}

}
