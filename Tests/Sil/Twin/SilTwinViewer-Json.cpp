/*
Filename: Tests/Sil/Twin/SilTwinViewer-Json.cpp
Description: JSON primitives of the Digital Twin viewer serialization: string
escaping, fault activity and the logical component health map.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTwinViewer;

import std;

import SafetyManager;
import SilEvents;
import SilFaultScenario;

namespace sim::test::sil {

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

}