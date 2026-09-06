/*
Filename: Src/Embedded/Hil/HilRunner-Loop.cpp
Description: Top-level real-time loop, fault-application step and deadline handling of the
HIL runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FlightController;
import HilConfig;
import HilEvents;
import CommsBus;
import FaultInjectors;
import HilClock;
import HilRunnerContext;
import HilTiming;
import SilFaultScenario;
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

/*
Executes the closed loop on an absolute wall-clock schedule: simulation time advances by
dt each step regardless of wall time, while each cycle is bounded by a fixed deadline. A
step that completes past its deadline is recorded (and may abort per policy); the next
deadline is always scheduled relative to the original epoch, so overruns are carried
rather than accumulated as drift via relative sleeps.
*/
void HilRunner::execute(HilRunContext& ctx)
{
    const HilConfig& cfg = ctx.config;
    const std::uint64_t period_us = static_cast<std::uint64_t>(std::llround(cfg.dt_s * 1.0e6));
    const std::uint64_t total_steps = hil_step_count(cfg);

    ctx.start_wall_us = ctx.clock->nowUs();
    ctx.next_deadline_us = ctx.start_wall_us + period_us;
    record_run_start(ctx);

    for (ctx.step = 0; ctx.step < total_steps; ++ctx.step) {
        ctx.time = static_cast<std::float64_t>(ctx.step) * cfg.dt_s;
        ctx.sim_ts_us = ctx.step * period_us;

        HilStepTiming timing{};
        timing.scheduled_us = ctx.start_wall_us + ctx.step * period_us;
        timing.actual_start_us = ctx.clock->nowUs();

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
        timing.aircraft_update_us = ctx.clock->nowUs();
        timing.step_completion_us = ctx.clock->nowUs();

        ctx.timing.record(timing, period_us);
        if (timing.deadline_missed(period_us)) {
            handle_deadline(ctx, timing);
            if (ctx.aborted_on_deadline) {
                break;
            }
        }

        if (cfg.real_time_pacing) {
            ctx.clock->sleepUntilUs(ctx.next_deadline_us);
        }
        ctx.next_deadline_us += period_us;
    }

    finalize(ctx);
    record_run_end(ctx);
    stream_live_output(ctx);
}

void HilRunner::setLiveStream(std::ostream& out)
{
    live_out_ = &out;
}

/*
Runs the configured scenario(s) and returns the structured outcome, or a typed error.
*/
std::expected<HilRunOutput, HilError> HilRunner::run(std::span<const sim::sil::FaultScenario> scenarios)
{
    return makeContext(config_, scenarios).and_then([this](std::unique_ptr<HilRunContext> ctx) {
        ctx->live_out = live_out_;
        execute(*ctx);
        HilRunOutput output{};
        output.result = ctx->result;
        output.config = ctx->config;
        output.events = ctx->trace.takeEvents();
        output.telemetry = std::move(ctx->telemetry_recorder.samples);
        output.ground_truth = std::move(ctx->telemetry_recorder.truth_samples);
        return std::expected<HilRunOutput, HilError>{std::move(output)};
    });
}

}
