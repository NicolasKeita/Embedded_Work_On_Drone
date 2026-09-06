/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Inject.cpp
Description: Fault-injection step and deadline-miss handling of the HIL runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import CommsBus;
import FaultInjectors;
import HilClock;
import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilRunnerEvents;
import HilTiming;
import SilTypes;

namespace sim::hil {

/*
Applies the timed fault injectors to the environment for this step. The environment is
reset to its nominal baseline each step and then re-injected by active injectors so
that temporary faults are genuinely cleared when their window ends. The FC1-FC2/actuator
link parameters are mirrored into the reused CommsBus so detection matches the SIL
baseline.
*/
void apply_injectors(HilRunContext& ctx)
{
    ctx.env = sim::sil::SimulationState{};
    for (std::size_t i = 0; i < ctx.injector_count; ++i) {
        ctx.injectors[i].inject(ctx.env, ctx.time);
    }
    ctx.comms.set_link(ctx.env.comms_link_up, ctx.env.comms_loss_probability);
    record_fault_events(ctx);
}

/*
Records a deadline miss and applies the configured policy: Warn records and continues,
Fail records and will force a FAIL verdict at finalization, Abort stops the loop.
*/
void handle_deadline(HilRunContext& ctx, const HilStepTiming& timing)
{
    const std::uint64_t period_us = static_cast<std::uint64_t>(std::llround(ctx.config.dt_s * 1e6));

    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "HIL_RUNNER",
                              .type = HilEventType::DeadlineMissed,
                              .severity = HilEventSeverity::Warning,
                              .detail = deadline_policy_name(ctx.config.deadline_policy),
                              .reason = "real-time deadline exceeded",
                              .sequence = ctx.step,
                              .has_sequence = true,
                              .duration_us = timing.lateness_us(period_us),
                              .has_duration = true});
    if (ctx.config.deadline_policy == DeadlinePolicy::Abort) {
        ctx.aborted_on_deadline = true;
    }
}

}
