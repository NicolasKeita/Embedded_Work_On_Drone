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

    SilEvent transition{.timestamp = ctx.time,
                        .source = "FC2",
                        .type = SilEventType::SafetyStateTransition,
                        .severity = EventSeverity::Info,
                        .previous_state = health_state_name(ctx.previous_health),
                        .new_state = health_state_name(current),
                        .reason = current == HealthState::HEALTHY ? "all detection flags clear" : "detection flags raised"};
    ctx.trace.record(transition);
}

/*
Records the recovery completion when health returns to HEALTHY after a fault,
together with the heartbeat supervision recovery marker.
*/
void record_recovery_end(RunContext& ctx, sim::safety::HealthState current)
{
    if (ctx.result.detection_time < 0.0 || current != HealthState::HEALTHY || ctx.result.recovery_time >= 0.0) {
        return;
    }
    ctx.result.recovery_time = ctx.time;
    ctx.result.recovery_successful = true;

    SilEvent recovery_end{.timestamp = ctx.time,
                          .source = "FC2",
                          .type = SilEventType::RecoveryEnd,
                          .severity = EventSeverity::Info,
                          .reason = "health restored"};

    SilEvent supervision_recovery = recovery_end;
    supervision_recovery.type = SilEventType::SupervisionRecovery;
    ctx.trace.record(recovery_end);
    ctx.trace.record(supervision_recovery);
}

}
