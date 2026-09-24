/*
Filename: Tests/Sil/Twin/SilTwinViewer-Snapshot.cpp
Description: TwinSnapshot serialization of one SIL telemetry sample for the viewer.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTwinViewer;

import std;

import FlightControllerTypes;
import SafetyManager;
import SilEvents;
import SilFaultScenario;
import SilTelemetry;
import SilRunnerContext;
import SilRuntimeConfig;

namespace sim::test::sil {

/* Writes at most twelve scenario events that have occurred by the current sample. */
void write_recent_events(std::ostream&                       out,
                         std::span<const sim::sil::SilEvent> events,
                         std::float64_t                      time)
{
    std::size_t eligible_count = 0U;

    for (const sim::sil::SilEvent& event : events) {
        if (event.timestamp <= time) {
            ++eligible_count;
        }
    }
    const std::size_t first = eligible_count > 12U ? eligible_count - 12U : 0U;
    std::size_t eligible_index = 0U;
    bool wrote_event = false;
    out << '[';
    for (const sim::sil::SilEvent& event : events) {
        if (event.timestamp > time) {
            continue;
        }
        if (eligible_index < first) {
            ++eligible_index;
            continue;
        }
        if (wrote_event) {
            out << ',';
        }
        out << "{\"time_s\":" << event.timestamp << ",\"type\":";
        write_json_string(out, sim::sil::event_type_name(event.type));
        out << ",\"message\":";
        write_json_string(out, event.detail.empty() ? event.reason : event.detail);
        out << ",\"level\":";
        write_json_string(out, viewer_event_level(event.severity));
        out << '}';
        wrote_event = true;
        ++eligible_index;
    }
    out << ']';
}

/* Writes the state block of a snapshot: aircraft, target, actuators and mission. */
void write_state_json(std::ostream& out, const sim::sil::TelemetrySample& sample)
{
    const std::float64_t airspeed = std::hypot(sample.vx, sample.vy, sample.vz);

    out << "{\"x_m\":" << sample.x << ",\"y_m\":" << sample.y
        << ",\"z_m\":" << sample.altitude_m << ",\"altitude_m\":" << sample.altitude_m
        << ",\"pitch_rad\":" << sample.pitch_rad << ",\"roll_rad\":" << sample.roll_rad
        << ",\"airspeed_ms\":" << airspeed << "},\"target\":{\"x_m\":" << sample.target_x
        << ",\"y_m\":" << sample.target_y << ",\"altitude_m\":" << sample.target_z
        << "},\"actuators\":{\"rotor_rpm\":" << sample.actual_rpm
        << ",\"left_servo_deg\":" << sample.left_servo_deg
        << ",\"right_servo_deg\":" << sample.right_servo_deg << "},\"mission\":";
    write_json_string(out, sim::control::mission_state_name(
                               static_cast<sim::control::MissionState>(sample.mission_state)));
}

/* Writes one FC status block of a snapshot: status string plus component map. */
void write_fc_status(std::ostream& out, std::string_view status, std::string_view affected, bool failed)
{
    write_json_string(out, status);
    out << ",\"components\":";
    write_components(out, affected, failed);
    out << '}';
}

/* Serializes one SIL telemetry sample into the TwinSnapshot schema shared with HIL. */
void write_snapshot(std::ostream&                    out,
                    const sim::sil::ScenarioRecord&  record,
                    const sim::sil::TelemetrySample& sample)
{
    const bool active = fault_is_active(record, sample.time);
    const bool fc1_failed = active && record.scenario.failure_mode == sim::sil::FailureMode::FC1_UNAVAILABLE;
    const std::string_view affected = active ? affected_component(record.scenario.failure_mode)
                                             : std::string_view{};
    const auto safety_mode = static_cast<sim::safety::SafetyMode>(sample.safety_state);

    const auto config = sim::host::sil_config_for_scenario(record.name);
    const std::float64_t wind = sim::sil::wind_factor(config, sample.time);
    out << std::setprecision(8) << "{\"source\":\"SIL\",\"time_s\":" << sample.time
        << ",\"wind\":{\"x_mps\":" << config.wind_x_mps * wind
        << ",\"y_mps\":" << config.wind_y_mps * wind << "},\"aircraft\":";
    write_state_json(out, sample);
    out << ",\"health\":";
    write_json_string(out, active ? "DEGRADED" : "HEALTHY");
    out << ",\"safety_mode\":";
    write_json_string(out, sim::safety::safety_mode_name(safety_mode));
    out << ",\"active_fault\":";
    if (active) {
        write_json_string(out, sim::sil::failure_mode_name(record.scenario.failure_mode));
    }
    else {
        out << "null";
    }
    out << ",\"fc1\":{\"status\":";
    write_fc_status(out, fc1_failed ? "OFFLINE" : "ONLINE", affected, fc1_failed);
    out << ",\"fc2\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"hil\":{\"loop_hz\":" << 1.0 / config.dt
        << ",\"deadline_misses\":0},\"events\":";
    write_recent_events(out, record.events, sample.time);
    out << '}';
}

}
