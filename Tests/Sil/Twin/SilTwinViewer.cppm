/*
Filename: Tests/Sil/Twin/SilTwinViewer.cppm
Description: Interface of the single-scenario SIL telemetry stream for the localhost
Digital Twin viewer. The TwinSnapshot serialization and live replay/streaming helpers
are split across SilTwinViewer-Json.cpp, SilTwinViewer-Snapshot.cpp and
SilTwinViewer-Stream.cpp.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilTwinViewer;

import std;

import MissionRunner;
import SafetyManager;
import Scenarios;
import SilEvents;
import SilFaultScenario;
import SilReporting;
import SilTelemetry;
import TestHarness;
import TwinWebSocketPublisher;

export namespace sim::test::sil {

/* Replays one completed SIL scenario to the localhost viewer at its recorded telemetry cadence. */
void stream_sil_twin(const sim::sil::ScenarioRecord& record);

/* Runs one functional SIL mission while streaming its compressed progress to the viewer. */
void run_functional_sil_twin(const sim::test::ScenarioEntry& entry,
                             sim::test::TestHarness& runner,
                             std::float64_t hover_rpm);

}

namespace sim::test::sil {

/* Writes a JSON string using the escaping required by the viewer protocol. */
void write_json_string(std::ostream& out, std::string_view value);

/* Reports whether the configured scenario fault is active at the sample time. */
bool fault_is_active(const sim::sil::ScenarioRecord& record, std::float64_t time) noexcept;

/* Maps SIL event severity to the three levels accepted by the viewer. */
std::string_view viewer_event_level(sim::sil::EventSeverity severity) noexcept;

/* Writes the standard logical component map, degrading the component targeted by the active fault. */
void write_components(std::ostream& out, std::string_view affected, bool failed);

/* Resolves the viewer component associated with a SIL failure mode. */
std::string_view affected_component(sim::sil::FailureMode mode) noexcept;

/* Writes at most twelve scenario events that have occurred by the current sample. */
void write_recent_events(std::ostream& out,
                         std::span<const sim::sil::SilEvent> events,
                         std::float64_t time);

/* Writes the state block of a snapshot: aircraft, target, actuators and mission. */
void write_state_json(std::ostream& out, const sim::sil::TelemetrySample& sample);

/* Writes one FC status block of a snapshot: status string plus component map. */
void write_fc_status(std::ostream& out, std::string_view status, std::string_view affected, bool failed);

/* Serializes one SIL telemetry sample into the TwinSnapshot schema shared with HIL. */
void write_snapshot(std::ostream& out,
                    const sim::sil::ScenarioRecord& record,
                    const sim::sil::TelemetrySample& sample);

/* Publishes one compressed functional-mission sample using the shared TwinSnapshot schema. */
void publish_functional_sample(const sim::test::MissionViewerSample& sample, void* opaque_context);

/* Process-local publisher context of the functional mission stream. */
struct FunctionalViewerContext {
    sim::hil::TwinWebSocketPublisher publisher{};
};

}
