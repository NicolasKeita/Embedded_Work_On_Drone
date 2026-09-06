/*
Filename: Src/Embedded/Hil/HilEvents.cpp
Description: HIL event type/severity/category naming and the HilTrace recorder.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilEvents;

import std;


namespace sim::hil {

void HilTrace::record(HilEvent event)
{
    events_.push_back(event);
}

std::span<const HilEvent> HilTrace::events() const noexcept
{
    return events_;
}

std::vector<HilEvent> HilTrace::takeEvents() noexcept
{
    return std::move(events_);
}

std::string_view event_type_name(HilEventType type) noexcept
{
    switch (type) {
    case HilEventType::HilRunStart:
        return "HIL_RUN_START";
    case HilEventType::HilRunEnd:
        return "HIL_RUN_END";
    case HilEventType::MissionStateTransition:
        return "MISSION_STATE_TRANSITION";
    case HilEventType::MissionComplete:
        return "MISSION_COMPLETE";
    case HilEventType::MissionAborted:
        return "MISSION_ABORTED";
    case HilEventType::FaultInjected:
        return "FAULT_INJECTED";
    case HilEventType::FaultCleared:
        return "FAULT_CLEARED";
    case HilEventType::FaultDetected:
        return "FAULT_DETECTED";
    case HilEventType::SafetyStateTransition:
        return "SAFETY_STATE_TRANSITION";
    case HilEventType::HeartbeatTimeout:
        return "HEARTBEAT_TIMEOUT";
    case HilEventType::Fc1Failure:
        return "FC1_FAILURE";
    case HilEventType::DeadlineMissed:
        return "DEADLINE_MISSED";
    case HilEventType::HilStepError:
        return "HIL_STEP_ERROR";
    case HilEventType::CommandSent:
        return "COMMAND_SENT";
    case HilEventType::CommandDropped:
        return "COMMAND_DROPPED";
    case HilEventType::CommandReceived:
        return "COMMAND_RECEIVED";
    }
    return "UNKNOWN";
}

std::string_view event_severity_name(HilEventSeverity severity) noexcept
{
    switch (severity) {
    case HilEventSeverity::Info:
        return "INFO";
    case HilEventSeverity::Warning:
        return "WARNING";
    case HilEventSeverity::Error:
        return "ERROR";
    }
    return "INFO";
}

std::string_view event_category(HilEventType type) noexcept
{
    switch (type) {
    case HilEventType::MissionStateTransition:
    case HilEventType::MissionComplete:
    case HilEventType::MissionAborted:
        return "MISSION";
    case HilEventType::FaultInjected:
    case HilEventType::FaultCleared:
    case HilEventType::FaultDetected:
    case HilEventType::Fc1Failure:
        return "FAULT";
    case HilEventType::SafetyStateTransition:
    case HilEventType::HeartbeatTimeout:
        return "SAFETY";
    case HilEventType::DeadlineMissed:
    case HilEventType::HilStepError:
        return "TIMING";
    case HilEventType::HilRunStart:
    case HilEventType::HilRunEnd:
        return "HIL";
    default:
        return "COMM";
    }
}

bool is_report_event(HilEventType type) noexcept
{
    switch (type) {
    case HilEventType::CommandSent:
    case HilEventType::CommandDropped:
    case HilEventType::CommandReceived:
        return false;
    default:
        return true;
    }
}

}
