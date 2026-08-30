/*
Filename: Src/SIL/SilRunner-Run.cpp
Description: Top-level SIL simulation loop driving the per-step pipeline.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import SilTypes;

namespace sim::sil {

/*
Executes the per-step pipeline (injection, FC1, monitoring, actuators, metrics)
until the configured duration is reached, then finalizes the result.
*/
void SILRunner::execute(RunContext& ctx)
{
    const double dt = ctx.config.dt;

    for (ctx.time = 0.0; ctx.time <= ctx.config.duration_s + 0.5 * dt; ctx.time += dt) {
        apply_injectors(ctx);
        update_fc1(ctx);
        update_monitoring(ctx);
        apply_actuators(ctx);
        update_metrics(ctx);
    }
    finalize(ctx);
}

/*
Runs the full SIL simulation for the given fault scenarios: builds the run
context then executes the per-step pipeline, chaining both stages monadically.
*/
std::expected<SimulationResult, SilError> SILRunner::run(std::span<const FaultScenario> scenarios)
{
    return make_context(config_, scenarios).and_then([](RunContext&& ctx) {
        execute(ctx);
        return std::expected<SimulationResult, SilError>{ctx.result};
    });
}

}
