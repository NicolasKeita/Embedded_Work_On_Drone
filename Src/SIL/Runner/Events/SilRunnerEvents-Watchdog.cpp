/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Watchdog.cpp
Description: Watchdog timeout and recovery event recording.

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

using sim::safety::FaultDomain;
using sim::safety::HealthReport;

/*
Orchestrates the per-flag watchdog recording chain and the fault detection.
*/
void record_watchdog_and_detection(RunContext& ctx, const HealthReport& report)
{
    record_watchdog_events(ctx, report);
    record_recovery_start(ctx, report);
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        ctx.previous_flags[index] = report.flags[index].raised;
    }
    record_detection(ctx, report);
}

/*
Emits watchdog and message timeout events and updates the statistics.
*/
void record_watchdog_events(RunContext& ctx, const sim::safety::HealthReport& report)
{
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        const bool raised = report.flags[index].raised;
        const FaultDomain domain = static_cast<FaultDomain>(index);

        if (!raised || ctx.previous_flags[index]
            || (domain != FaultDomain::FC1Heartbeat && domain != FaultDomain::Communication)) {
            continue;
        }
        SilEvent timeout{.timestamp = ctx.time,
                         .source = "FC2",
                         .type = SilEventType::WatchdogTimeout,
                         .severity = EventSeverity::Warning,
                         .detail = fault_domain_name(domain),
                         .reason = "supervised link silent"};
        ctx.trace.record(timeout);

        SilEvent message_timeout = timeout;
        message_timeout.type = SilEventType::MessageTimeout;
        message_timeout.reason = {};
        ctx.trace.record(message_timeout);

        ctx.comms_stats.record_timeout();
        if (!ctx.result.watchdog_triggered) {
            ctx.result.watchdog_triggered = true;
            ctx.result.watchdog_trigger_time =
                report.flags[index].raised_time >= 0.0 ? report.flags[index].raised_time : ctx.time;
        }
    }
}

/*
Emits the recovery start event on a fault flag falling edge.
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
                          .detail = fault_domain_name(static_cast<FaultDomain>(index)),
                          .reason = "fault flag cleared"};
        ctx.trace.record(recovery);
    }
}

}
