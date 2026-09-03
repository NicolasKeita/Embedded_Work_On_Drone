/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Health.cpp
Description: Health state transition and recovery completion event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import HealthMonitor;
import SilEvents;
import SilRunnerContext;
import SilTypes;

namespace sim::sil {

using sim::safety::HealthState;

/*
Records one health state transition with its previous/new states.
*/
void record_health_transition(RunContext& ctx, sim::safety::HealthState current)
{
    if (current == ctx.previous_health) {
        return;
    }

    SilEvent transition;

    transition.timestamp = ctx.time;
    transition.source = "FC2";
    transition.type = SilEventType::SafetyStateTransition;
    transition.severity = EventSeverity::Info;
    transition.previous_state = health_state_name(ctx.previous_health);
    transition.new_state = health_state_name(current);
    transition.reason = current == HealthState::HEALTHY ? "all flags clear" : "fault flags raised";
    ctx.trace.record(transition);
}

/*
Records the recovery completion when health returns to HEALTHY after a fault.
*/
void record_recovery_end(RunContext& ctx, sim::safety::HealthState current)
{
    if (ctx.result.detection_time < 0.0 || current != HealthState::HEALTHY || ctx.result.recovery_time >= 0.0) {
        return;
    }
    ctx.result.recovery_time = ctx.time;
    ctx.result.recovery_successful = true;

    SilEvent recovery_end;
    recovery_end.timestamp = ctx.time;
    recovery_end.source = "FC2";
    recovery_end.type = SilEventType::RecoveryEnd;
    recovery_end.severity = EventSeverity::Info;
    recovery_end.reason = "health restored";

    SilEvent watchdog_recovery = recovery_end;
    watchdog_recovery.type = SilEventType::WatchdogRecovery;
    ctx.trace.record(recovery_end);
    ctx.trace.record(watchdog_recovery);
}

}