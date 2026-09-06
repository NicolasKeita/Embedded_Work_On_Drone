/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Faults.cpp
Description: Fault detection, FC1 failure and fault injection/clearing event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerEvents;

import std;

import FaultInjectors;
import HealthMonitor;
import HilClock;
import HilEvents;
import HilRunnerContext;
import SilFaultScenario;

namespace sim::hil {

namespace {
    sim::sil::FaultType active_fault_type(const HilRunContext& ctx)
    {
        for (std::size_t i = 0; i < ctx.injector_count; ++i) {
            if (ctx.injectors[i].is_active(ctx.time)) {
                return ctx.injectors[i].scenario().fault_type;
            }
        }
        return sim::sil::FaultType::None;
    }

    void record_fault_injected(HilRunContext& ctx, sim::sil::FaultType active_type)
    {
        ctx.fault_active = true;
        ctx.last_fault_type = active_type;
        if (!ctx.fault_recorded) {
            ctx.fault_recorded = true;
            ctx.result.fault_injected_time = ctx.time;
        }
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultInjected,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::fault_type_name(active_type),
                                  .reason = "fault injected into the HIL data path"});
        if (active_type == sim::sil::FaultType::FC1Failure && ctx.fc1_was_alive) {
            ctx.fc1_was_alive = false;
            record_fc1_failure_event(ctx);
        }
    }

    void record_fault_cleared(HilRunContext& ctx)
    {
        ctx.fault_active = false;
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultCleared,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::fault_type_name(ctx.last_fault_type),
                                  .reason = "fault window ended"});
    }
}

void record_detection(HilRunContext& ctx, const sim::safety::HealthReport& report)
{
    const std::float64_t detection = report.first_detection_time();
    if (detection < 0.0 || ctx.result.detection_time >= 0.0) {
        return;
    }
    ctx.result.detection_time = detection;
    ctx.result.first_fault_domain = report.first_fault_domain();
    ctx.result.fault_detected = true;
    if (ctx.detection_recorded) {
        return;
    }
    ctx.detection_recorded = true;
    ctx.trace.record(HilEvent{.sim_time_s = detection,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC2",
                              .type = HilEventType::FaultDetected,
                              .severity = HilEventSeverity::Info,
                              .detail = sim::safety::fault_domain_name(ctx.result.first_fault_domain),
                              .reason = "fault detected"});
    if (ctx.result.first_fault_domain == sim::safety::FaultDomain::FC1Heartbeat) {
        ctx.trace.record(HilEvent{.sim_time_s = detection,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FC2",
                                  .type = HilEventType::HeartbeatTimeout,
                                  .severity = HilEventSeverity::Warning,
                                  .reason = "FC1 heartbeat / actuator-link timeout"});
    }
}

void record_fc1_failure_event(HilRunContext& ctx)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC1",
                              .type = HilEventType::Fc1Failure,
                              .severity = HilEventSeverity::Warning,
                              .detail = "FC1_FAILURE"});
}

void record_fault_events(HilRunContext& ctx)
{
    const sim::sil::FaultType active_type = active_fault_type(ctx);

    if (active_type != sim::sil::FaultType::None && !ctx.fault_active) {
        record_fault_injected(ctx, active_type);
    }
    else if (active_type == sim::sil::FaultType::None && ctx.fault_active) {
        record_fault_cleared(ctx);
    }
}

}