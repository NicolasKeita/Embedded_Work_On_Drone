/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Loop.cpp
Description: Top-level real-time loop of the HIL runner: closed-loop execution on an
absolute wall-clock schedule with per-cycle deadline checks and graceful stop handling.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import HilClock;
import HilConfig;
import HilReport;
import HilRunnerContext;
import HilRunnerEvents;
import HilRunnerSafety;
import HilTiming;

namespace sim::hil {

namespace {
    /*
    Executes one closed-loop cycle: fault injection, actuator exchange, host-side
    safety evaluation, actuator application, metric aggregation, live streaming and the
    deadline-miss check against the cycle budget.
    */
    void run_step(HilRunContext& ctx, std::uint64_t period_us)
    {
        const HilConfig& cfg = ctx.config;

        ctx.time = static_cast<std::float64_t>(ctx.step) * cfg.dt_s;
        ctx.sim_ts_us = ctx.step * period_us;

        HilStepTiming timing{.scheduled_us = ctx.start_wall_us + ctx.step * period_us,
                             .actual_start_us = ctx.clock.nowUs()};

        apply_injectors(ctx);
        exchange_actuators(ctx);
        update_health_and_safety(ctx);
        apply_actuators(ctx);
        timing.sensor_send_us = ctx.sensor_send_wall_us;
        timing.actuator_receive_us = ctx.actuator_receive_wall_us;
        if (ctx.this_fc.ok) {
            timing.fc_receive_us = static_cast<std::int64_t>(ctx.this_fc.fc_receive_wall_us);
            timing.fc_send_us = static_cast<std::int64_t>(ctx.this_fc.fc_send_wall_us);
        }
        update_metrics(ctx);
        stream_live_output(ctx);
        timing.aircraft_update_us = ctx.clock.nowUs();
        timing.step_completion_us = ctx.clock.nowUs();

        ctx.timing.record(timing, period_us);
        if (timing.deadline_missed(period_us)) {
            handle_deadline(ctx, timing);
        }
    }
}

/*
Executes the closed loop on an absolute wall-clock schedule: simulation time advances by
dt each step regardless of wall time, while each cycle is bounded by a fixed deadline. A
step that completes past its deadline is recorded (and may abort per policy); the next
deadline is always scheduled relative to the original epoch, so overruns are carried
rather than accumulated as drift via relative sleeps.
*/
void HilRunner::execute(HilRunContext& ctx)
{
    const HilConfig&    cfg = ctx.config;
    const std::uint64_t period_us = static_cast<std::uint64_t>(std::llround(cfg.dt_s * 1.0e6));
    const std::uint64_t total_steps = hil_step_count(cfg);

    ctx.start_wall_us = ctx.clock.nowUs();
    ctx.next_deadline_us = ctx.start_wall_us + period_us;
    record_run_start(ctx);

    for (ctx.step = 0; ctx.step < total_steps; ++ctx.step) {
        if (stop_requested_ != nullptr && *stop_requested_ != 0) {
            break;
        }
        run_step(ctx, period_us);
        if (ctx.aborted_on_deadline) {
            break;
        }
        ctx.clock.sleepUntilUs(ctx.next_deadline_us);
        ctx.next_deadline_us += period_us;
    }

    reset_embedded_supervision(ctx);
    finalize(ctx);
    record_run_end(ctx);
    stream_live_output(ctx);
}

}
