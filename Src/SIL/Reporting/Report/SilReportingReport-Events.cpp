/*
Filename: Src/SIL/Reporting/Report/SilReportingReport-Events.cpp
Description: Important discrete events section of the Markdown report (no heartbeat noise).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTypes;

namespace sim::sil {

namespace {

/*
True when the event belongs to the human-readable report: heartbeat and
per-message traffic stay in the detailed trace only.
*/
bool is_report_event(SilEventType type)
{
    switch (type) {
    case SilEventType::MessageGenerated:
    case SilEventType::MessageDelivered:
    case SilEventType::MessageDropped:
    case SilEventType::MessageTimeout:
    case SilEventType::HeartbeatSent:
    case SilEventType::HeartbeatDelivered:
    case SilEventType::HeartbeatDropped:
    case SilEventType::WatchdogKick:
        return false;
    default:
        return true;
    }
}

/*
Short category label of a report event row.
*/
std::string_view event_category(SilEventType type)
{
    switch (type) {
    case SilEventType::FaultInjected:
    case SilEventType::FaultCleared:
    case SilEventType::FaultDetected:
    case SilEventType::FaultClassified:
    case SilEventType::SensorFault:
    case SilEventType::ActuatorFault:
        return "FAULT";
    case SilEventType::SafetyResponse:
    case SilEventType::SafetyStateTransition:
    case SilEventType::WatchdogTimeout:
        return "SAFETY";
    case SilEventType::MissionStateTransition:
        return "MISSION";
    case SilEventType::RecoveryStart:
    case SilEventType::RecoveryEnd:
    case SilEventType::WatchdogRecovery:
        return "RECOVERY";
    case SilEventType::FCStartup:
    case SilEventType::FCShutdown:
    case SilEventType::FCFailure:
        return "FC";
    default:
        return "SIL";
    }
}

/*
Streams the description of one report event: type, detail, state transition and
reason when present.
*/
void write_event_description(std::ostream& out, const SilEvent& event)
{
    out << event_type_name(event.type);
    if (!event.detail.empty()) {
        out << ' ' << event.detail;
    }
    if (!event.previous_state.empty()) {
        out << ' ' << event.previous_state << " -> " << event.new_state;
    }
    if (!event.reason.empty()) {
        out << " (" << event.reason << ')';
    }
    if (event.has_value) {
        out << " value=";
        write_metric(out, event.value);
    }
    if (event.has_duration) {
        out << " duration=";
        write_seconds(out, event.duration_s);
        out << "s";
    }
}

}

/*
Writes the important discrete events of one scenario section (no heartbeat or
per-message traffic).
*/
void write_event_table(std::ostream& out, std::span<const SilEvent> events)
{
    out << "| t(s) | Categorie | Evenement |\n|---|---|---|\n";
    for (const SilEvent& event : events) {
        if (!is_report_event(event.type)) {
            continue;
        }
        out << "| ";
        write_seconds(out, event.timestamp);
        out << " | " << event_category(event.type) << " | ";
        write_event_description(out, event);
        out << " |\n";
    }
}

}
