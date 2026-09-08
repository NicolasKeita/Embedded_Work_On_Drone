/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Supervision.cpp
Description: Heartbeat/link supervision timeout and recovery event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import CommsBus;
import HealthMonitor;
import SilEvents;
import SilRunnerContext;
import SilTypes;

namespace sim::sil {

using sim::safety::DetectionEvent;
using sim::safety::HealthReport;

/*
Orchestrates the per-flag supervision recording chain and the fault detection.
*/
void record_supervision_and_detection(RunContext& ctx, const HealthReport& report)
{
    record_supervision_events(ctx, report);
    record_recovery_start(ctx, report);
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        ctx.previous_flags[index] = report.flags[index].raised;
    }
    record_detection(ctx, report);
}

/*
Emits the supervision and message timeout events and updates the statistics.
The supervision is the remote FC1 heartbeat/link liveness monitoring (not a
local task watchdog): the event detail names the detection event that fired.
*/
void record_supervision_events(RunContext& ctx, const sim::safety::HealthReport& report)
{
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        const bool           raised = report.flags[index].raised;
        const DetectionEvent event = static_cast<DetectionEvent>(index);

        if (!raised || ctx.previous_flags[index]
            || (event != DetectionEvent::FC1_HEARTBEAT_TIMEOUT && event != DetectionEvent::COMMUNICATION_TIMEOUT)) {
            continue;
        }
        SilEvent timeout{.timestamp = ctx.time,
                         .source = "FC2",
                         .type = SilEventType::SupervisionTimeout,
                         .severity = EventSeverity::Warning,
                         .detail = detection_event_name(event),
                         .reason = "supervised FC1->FC2 link silent"};
        ctx.trace.record(timeout);

        SilEvent message_timeout = timeout;
        message_timeout.type = SilEventType::MessageTimeout;
        message_timeout.reason = {};
        ctx.trace.record(message_timeout);

        ctx.comms_stats.record_timeout();
        if (!ctx.result.supervision_triggered) {
            ctx.result.supervision_triggered = true;
            ctx.result.supervision_trigger_time =
                report.flags[index].raised_time >= 0.0 ? report.flags[index].raised_time : ctx.time;
        }
    }
}

/*
Emits the recovery start event on a detection flag falling edge.
*/
void record_recovery_start(RunContext& ctx, const HealthReport& report)
{
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        if (report.flags[index].raised || !ctx.previous_flags[index] || !ctx.result.fault_detected
            || ctx.recovery_recorded) {
            continue;
        }
        ctx.recovery_recorded = true;
        ctx.result.recovery_attempted = true;

        SilEvent recovery{.timestamp = ctx.time,
                          .source = "FC2",
                          .type = SilEventType::RecoveryStart,
                          .severity = EventSeverity::Info,
                          .detail = detection_event_name(static_cast<DetectionEvent>(index)),
                          .reason = "detection flag cleared"};
        ctx.trace.record(recovery);
    }
}

}
