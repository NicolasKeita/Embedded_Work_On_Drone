/*
Filename: Tests/Sil/SilTwinViewer.cpp
Description: Serialization and visible-rate replay of one SIL scenario to the localhost Digital Twin viewer.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTwinViewer;

import std;

import Aircraft;
import FlightControllerTypes;
import MissionRunner;
import SafetyManager;
import Scenarios;
import SilEvents;
import SilFaultScenario;
import SilReporting;
import SilTelemetry;
import TestHarness;
import TwinWebSocketPublisher;

namespace sim::test::sil {

namespace {

/* Writes a JSON string using the escaping required by the viewer protocol. */
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

/* Reports whether the configured scenario fault is active at the sample time. */
bool fault_is_active(const sim::sil::ScenarioRecord& record, std::float64_t time) noexcept
{
    if (record.scenario.failure_mode == sim::sil::FailureMode::NONE || time < record.scenario.start_time) {
        return false;
    }
    return record.scenario.duration <= 0.0 || time < record.scenario.start_time + record.scenario.duration;
}

/* Maps SIL event severity to the three levels accepted by the viewer. */
std::string_view viewer_event_level(sim::sil::EventSeverity severity) noexcept
{
    if (severity == sim::sil::EventSeverity::Error) {
        return "critical";
    }
    return severity == sim::sil::EventSeverity::Warning ? "warn" : "info";
}

/* Writes the standard logical component map, degrading the component targeted by the active fault. */
void write_components(std::ostream& out, std::string_view affected, bool failed)
{
    constexpr std::array<std::string_view, 7> components{
        "mcu", "transport", "sensors", "control", "actuators", "supervision", "safety"};
    out << '{';
    for (std::size_t index = 0; index < components.size(); ++index) {
        if (index != 0U) {
            out << ',';
        }
        write_json_string(out, components[index]);
        out << ':';
        write_json_string(out, components[index] == affected ? (failed ? "FAILED" : "DEGRADED") : "HEALTHY");
    }
    out << '}';
}

/* Resolves the viewer component associated with a SIL failure mode. */
std::string_view affected_component(sim::sil::FailureMode mode) noexcept
{
    if (mode == sim::sil::FailureMode::INVALID_SENSOR_DATA) {
        return "sensors";
    }
    if (mode == sim::sil::FailureMode::ACTUATOR_DEGRADED) {
        return "actuators";
    }
    if (mode == sim::sil::FailureMode::FC1_UNAVAILABLE) {
        return "mcu";
    }
    if (mode == sim::sil::FailureMode::FC_COMMUNICATION_LOSS
        || mode == sim::sil::FailureMode::COMMUNICATION_DEGRADED) {
        return "transport";
    }
    return {};
}

/* Writes at most twelve scenario events that have occurred by the current sample. */
void write_recent_events(std::ostream& out,
                         std::span<const sim::sil::SilEvent> events,
                         std::float64_t time)
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

/* Serializes one SIL telemetry sample into the TwinSnapshot schema shared with HIL. */
void write_snapshot(std::ostream& out,
                    const sim::sil::ScenarioRecord& record,
                    const sim::sil::TelemetrySample& sample)
{
    const bool active = fault_is_active(record, sample.time);
    const bool fc1_failed = active && record.scenario.failure_mode == sim::sil::FailureMode::FC1_UNAVAILABLE;
    const std::string_view affected = active ? affected_component(record.scenario.failure_mode) : std::string_view{};
    const auto safety_mode = static_cast<sim::safety::SafetyMode>(sample.safety_state);
    const std::float64_t airspeed = std::hypot(sample.vx, sample.vy, sample.vz);

    out << std::setprecision(8) << "{\"source\":\"SIL\",\"time_s\":" << sample.time
        << ",\"aircraft\":{\"x_m\":" << sample.x << ",\"y_m\":" << sample.y
        << ",\"z_m\":" << sample.altitude_m << ",\"altitude_m\":" << sample.altitude_m
        << ",\"pitch_rad\":" << sample.pitch_rad << ",\"roll_rad\":" << sample.roll_rad
        << ",\"airspeed_ms\":" << airspeed << "},\"target\":{\"x_m\":" << sample.target_x
        << ",\"y_m\":" << sample.target_y << ",\"altitude_m\":" << sample.target_z
        << "},\"actuators\":{\"rotor_rpm\":" << sample.actual_rpm
        << ",\"left_servo_deg\":" << sample.left_servo_deg
        << ",\"right_servo_deg\":" << sample.right_servo_deg << "},\"mission\":";
    write_json_string(out, sim::control::mission_state_name(
                               static_cast<sim::control::MissionState>(sample.mission_state)));
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
    write_json_string(out, fc1_failed ? "OFFLINE" : "ONLINE");
    out << ",\"components\":";
    write_components(out, affected, fc1_failed);
    out << "},\"fc2\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"hil\":{\"loop_hz\":100,\"deadline_misses\":0},\"events\":";
    write_recent_events(out, record.events, sample.time);
    out << '}';
}

struct FunctionalViewerContext {
    sim::hil::TwinWebSocketPublisher publisher{};
};

/* Publishes one compressed functional-mission sample using the shared TwinSnapshot schema. */
void publish_functional_sample(const sim::test::MissionViewerSample& sample, void* opaque_context)
{
    auto& context = *static_cast<FunctionalViewerContext*>(opaque_context);
    const AircraftState& aircraft = sample.aircraft;
    const std::float64_t airspeed = std::hypot(aircraft.vx, aircraft.vy, aircraft.vz);
    std::ostringstream out;

    out << std::setprecision(8) << "{\"source\":\"SIL\",\"time_s\":" << sample.time
        << ",\"aircraft\":{\"x_m\":" << aircraft.x << ",\"y_m\":" << aircraft.y
        << ",\"z_m\":" << aircraft.z << ",\"altitude_m\":" << aircraft.z
        << ",\"pitch_rad\":" << aircraft.pitch << ",\"roll_rad\":" << aircraft.roll
        << ",\"airspeed_ms\":" << airspeed << "},\"target\":{\"x_m\":" << sample.target.x
        << ",\"y_m\":" << sample.target.y << ",\"altitude_m\":" << sample.target.z
        << "},\"actuators\":{\"rotor_rpm\":" << aircraft.actual_rpm
        << ",\"left_servo_deg\":" << aircraft.actual_left_servo
        << ",\"right_servo_deg\":" << aircraft.actual_right_servo << "},\"mission\":";
    write_json_string(out, sim::control::mission_state_name(sample.mission));
    out << ",\"health\":\"HEALTHY\",\"safety_mode\":\"NORMAL\",\"active_fault\":null"
        << ",\"fc1\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"fc2\":{\"status\":\"ONLINE\",\"components\":";
    write_components(out, {}, false);
    out << "},\"hil\":{\"loop_hz\":100,\"deadline_misses\":0},\"events\":[]}";

    context.publisher.publish(out.str());
    std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

}

/* Replays one completed SIL scenario to the localhost viewer at its recorded telemetry cadence. */
void stream_sil_twin(const sim::sil::ScenarioRecord& record)
{
    if (record.telemetry.empty()) {
        return;
    }

    sim::hil::TwinWebSocketPublisher publisher{};
    std::cout << "\n--- Digital Twin SIL live replay: ws://localhost:8765/twin ---" << std::endl;
    std::float64_t previous_time = record.telemetry.front().time;
    for (const sim::sil::TelemetrySample& sample : record.telemetry) {
        const std::float64_t delay_s = std::clamp(sample.time - previous_time,
                                                  std::float64_t{0.0},
                                                  std::float64_t{0.1});
        std::this_thread::sleep_for(std::chrono::duration<std::float64_t>{delay_s});
        std::ostringstream snapshot;
        write_snapshot(snapshot, record, sample);
        publisher.publish(snapshot.str());
        previous_time = sample.time;
    }
}

/* Runs one functional SIL mission while streaming its compressed progress to the viewer. */
void run_functional_sil_twin(const sim::test::ScenarioEntry& entry,
                             sim::test::TestHarness& runner,
                             std::float64_t hover_rpm)
{
    FunctionalViewerContext context{};
    std::cout << "\n--- Digital Twin SIL live stream: ws://localhost:8765/twin ---" << std::endl;
    sim::test::set_mission_viewer_observer(
        sim::test::MissionViewerObserver{.callback = publish_functional_sample, .context = &context});
    entry.run(runner, hover_rpm, {});
    sim::test::set_mission_viewer_observer({});
}

}
