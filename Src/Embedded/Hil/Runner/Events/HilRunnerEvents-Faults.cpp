/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Faults.cpp
Description: Fault injection/clearing event recording of the HIL runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerEvents;

import std;

import FaultInjectors;
import HilClock;
import HilEvents;
import HilRunnerContext;
import SilFaultScenario;

namespace sim::hil {

namespace {
    sim::sil::FailureMode active_failure_mode(const HilRunContext& ctx)
    {
        for (std::size_t i = 0; i < ctx.injector_count; ++i) {
            if (ctx.injectors[i].is_active(ctx.time)) {
                return ctx.injectors[i].scenario().failure_mode;
            }
        }
        return sim::sil::FailureMode::NONE;
    }

    void record_fault_injected(HilRunContext& ctx, sim::sil::FailureMode active_mode)
    {
        ctx.fault_active = true;
        ctx.last_failure_mode = active_mode;
        if (!ctx.fault_recorded) {
            ctx.fault_recorded = true;
            ctx.result.fault_injected_time = ctx.time;
        }
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock.nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultInjected,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::failure_mode_name(active_mode),
                                  .reason = "failure mode injected into the HIL data path"});
        if (active_mode == sim::sil::FailureMode::FC1_UNAVAILABLE && ctx.fc1_was_alive) {
            ctx.fc1_was_alive = false;
            record_fc1_failure_event(ctx);
        }
    }

    void record_fault_cleared(HilRunContext& ctx)
    {
        ctx.fault_active = false;
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock.nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultCleared,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::failure_mode_name(ctx.last_failure_mode),
                                  .reason = "fault window ended"});
    }
}

void record_fault_events(HilRunContext& ctx)
{
    const sim::sil::FailureMode active_mode = active_failure_mode(ctx);

    if (active_mode != sim::sil::FailureMode::NONE && !ctx.fault_active) {
        record_fault_injected(ctx, active_mode);
    }
    else if (active_mode == sim::sil::FailureMode::NONE && ctx.fault_active) {
        record_fault_cleared(ctx);
    }
}

}
