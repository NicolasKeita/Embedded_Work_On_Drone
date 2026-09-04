/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Lifecycle.cpp
Description: Simulation lifecycle and FC1 failure event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import FlightController;
import SilEvents;
import SilRunnerContext;
import SilTypes;
import Telemetry;

namespace sim::sil {

/*
Emits the simulation start marker and the FC1 startup event at t = 0.
*/
void record_run_start(RunContext& ctx)
{
    SilEvent start{.timestamp = ctx.time,
                   .source = "SIL",
                   .type = SilEventType::SimulationStart,
                   .severity = EventSeverity::Info,
                   .reason = "SIL run started"};

    ctx.trace.record(start);

    SilEvent startup{.timestamp = ctx.time,
                     .source = "FC1",
                     .type = SilEventType::FCStartup,
                     .severity = EventSeverity::Info,
                     .reason = "FC1 online"};
    ctx.trace.record(startup);
}

/*
Emits the simulation end marker summarizing the final mission state.
*/
void record_run_end(RunContext& ctx)
{
    if (ctx.env.fc1_alive) {
        SilEvent shutdown{.timestamp = ctx.time,
                          .source = "FC1",
                          .type = SilEventType::FCShutdown,
                          .severity = EventSeverity::Info,
                          .reason = "simulation end"};
        ctx.trace.record(shutdown);
    }

    SilEvent end{.timestamp = ctx.time,
                 .source = "SIL",
                 .type = SilEventType::SimulationEnd,
                 .severity = EventSeverity::Info,
                 .detail = sim::control::mission_state_name(ctx.result.final_state),
                 .reason = ctx.result.mission_success ? "mission completed" : "mission not completed"};
    ctx.trace.record(end);
}

/*
Emits the FC1 failure event when the environment kills the flight computer.
*/
void record_fc1_failure(RunContext& ctx)
{
    SilEvent failure{.timestamp = ctx.time,
                     .source = "FC1",
                     .type = SilEventType::FCFailure,
                     .severity = EventSeverity::Warning,
                     .detail = "FC1_FAILURE"};

    ctx.trace.record(failure);
}

}
