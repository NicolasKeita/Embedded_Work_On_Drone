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
Runs the full SIL simulation for the given fault scenarios: builds the run
context, executes the per-step pipeline (injection, FC1, monitoring, actuators,
metrics) until the configured duration is reached, then finalizes the result.
*/
SimulationResult SILRunner::run(const std::vector<FaultScenario>& scenarios)
{
    RunContext ctx = make_context(config_, scenarios);
    const double dt = ctx.config.dt;
    for (ctx.time = 0.0; ctx.time <= ctx.config.duration_s + 0.5 * dt; ctx.time += dt) {
        apply_injectors(ctx);
        update_fc1(ctx);
        update_monitoring(ctx);
        apply_actuators(ctx);
        update_metrics(ctx);
    }
    finalize(ctx);
    return ctx.result;
}

}
