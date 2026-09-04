/*
Filename: Src/SIL/Reporting/Report/Events/SilReportingReport-EventRows.cpp
Description: Classification, category labels and descriptions of the Markdown report event rows.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReportingReport;

import std;

import SilEvents;
import SilFaultScenario;

namespace sim::sil {

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
Streams the description of one report event: type and subtype, target and
temporality profile of the fault injections, state transition and reason when
present, labelled numeric parameter and expected system response.
*/
void write_event_description(std::ostream& out, const SilEvent& event)
{
    out << event_type_name(event.type);
    if (!event.detail.empty()) {
        out << ' ' << event.detail;
    }
    if (!event.subtype.empty()) {
        out << " (" << event.subtype << ')';
    }
    write_event_target(out, event);
    if (event.has_profile) {
        out << " profile=" << fault_profile_name(event.profile);
    }
    if (!event.previous_state.empty()) {
        out << ' ' << event.previous_state << " -> " << event.new_state;
    }
    if (!event.reason.empty()) {
        out << " (" << event.reason << ')';
    }
    if (event.value_kind != FaultValueKind::None) {
        out << ' ';
        write_fault_value(out, event);
    }
    else if (event.has_value) {
        out << " value=";
        write_metric(out, event.value);
    }
    if (event.has_duration) {
        out << " duration=";
        write_seconds(out, event.duration_s);
        out << "s";
    }
    if (event.type == SilEventType::FaultInjected && !event.expected_behavior.empty()) {
        out << " expected=" << event.expected_behavior;
    }
}

}
