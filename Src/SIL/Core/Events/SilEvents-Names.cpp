/*
Filename: Src/SIL/Core/Events/SilEvents-Names.cpp
Description: Event naming, severity mapping and verbosity classification.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilEvents;

import std;

namespace sim::sil {

/*
Returns the machine-readable name of an event type for trace and report output.
*/
std::string_view event_type_name(SilEventType type)
{
    switch (type) {
    case SilEventType::SimulationStart:
        return "SIMULATION_START";
    case SilEventType::SimulationEnd:
        return "SIMULATION_END";
    case SilEventType::FaultInjected:
        return "FAULT_INJECTED";
    case SilEventType::FaultCleared:
        return "FAULT_CLEARED";
    case SilEventType::FaultDetected:
        return "FAULT_DETECTED";
    case SilEventType::FaultClassified:
        return "FAULT_CLASSIFIED";
    case SilEventType::SafetyResponse:
        return "SAFETY_RESPONSE";
    case SilEventType::SafetyStateTransition:
        return "SAFETY_STATE_TRANSITION";
    case SilEventType::MissionStateTransition:
        return "MISSION_STATE_TRANSITION";
    case SilEventType::WatchdogTimeout:
        return "WATCHDOG_TIMEOUT";
    case SilEventType::WatchdogKick:
        return "WATCHDOG_KICK";
    case SilEventType::WatchdogRecovery:
        return "WATCHDOG_RECOVERY";
    case SilEventType::FCStartup:
        return "FC_STARTUP";
    case SilEventType::FCShutdown:
        return "FC_SHUTDOWN";
    case SilEventType::FCFailure:
        return "FC_FAILURE";
    case SilEventType::MessageGenerated:
        return "MESSAGE_GENERATED";
    case SilEventType::MessageDelivered:
        return "MESSAGE_DELIVERED";
    case SilEventType::MessageDropped:
        return "MESSAGE_DROPPED";
    case SilEventType::MessageTimeout:
        return "MESSAGE_TIMEOUT";
    case SilEventType::HeartbeatSent:
        return "HEARTBEAT_SENT";
    case SilEventType::HeartbeatDelivered:
        return "HEARTBEAT_DELIVERED";
    case SilEventType::HeartbeatDropped:
        return "HEARTBEAT_DROPPED";
    case SilEventType::SensorFault:
        return "SENSOR_FAULT";
    case SilEventType::ActuatorFault:
        return "ACTUATOR_FAULT";
    case SilEventType::RecoveryStart:
        return "RECOVERY_START";
    case SilEventType::RecoveryEnd:
        return "RECOVERY_END";
    }
    return "UNKNOWN";
}

}