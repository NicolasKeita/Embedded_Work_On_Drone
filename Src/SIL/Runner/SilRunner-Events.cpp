/*
Filename: Src/SIL/Runner/SilRunner-Events.cpp
Description: Lifecycle and fault activation event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import FlightController;
import SilEvents;
import SilTypes;

namespace sim::sil {

/*
Emits the simulation start marker and the FC1 startup event at t = 0.
*/
void SILRunner::record_run_start(RunContext& ctx)
{
    SilEvent start;

    start.timestamp = ctx.time;
    start.source = "SIL";
    start.type = SilEventType::SimulationStart;
    start.severity = EventSeverity::Info;
    start.reason = "SIL run started";
    ctx.trace.record(start);

    SilEvent startup;

    startup.timestamp = ctx.time;
    startup.source = "FC1";
    startup.type = SilEventType::FCStartup;
    startup.severity = EventSeverity::Info;
    startup.reason = "FC1 online";
    ctx.trace.record(startup);
}

/*
Emits the simulation end marker summarizing the final mission state.
*/
void SILRunner::record_run_end(RunContext& ctx)
{
    if (ctx.env.fc1_alive) {
        SilEvent shutdown;

        shutdown.timestamp = ctx.time;
        shutdown.source = "FC1";
        shutdown.type = SilEventType::FCShutdown;
        shutdown.severity = EventSeverity::Info;
        shutdown.reason = "simulation end";
        ctx.trace.record(shutdown);
    }

    SilEvent end;

    end.timestamp = ctx.time;
    end.source = "SIL";
    end.type = SilEventType::SimulationEnd;
    end.severity = EventSeverity::Info;
    end.detail = sim::control::mission_state_name(ctx.result.final_state);
    end.reason = ctx.result.mission_success ? "mission completed" : "mission not completed";
    ctx.trace.record(end);
}

/*
Emits the FC1 failure event when the environment kills the flight computer.
*/
void SILRunner::record_fc1_failure(RunContext& ctx)
{
    SilEvent failure;

    failure.timestamp = ctx.time;
    failure.source = "FC1";
    failure.type = SilEventType::FCFailure;
    failure.severity = EventSeverity::Warning;
    failure.detail = "FC1_FAILURE";
    ctx.trace.record(failure);
}

/*
Emits the fault injection chain: the generic injection marker plus the typed
sensor/actuator events carrying their parameters. Called on a rising edge.
*/
void SILRunner::record_fault_activation(RunContext& ctx, const FaultScenario& scenario)
{
    SilEvent injected;

    injected.timestamp = ctx.time;
    injected.source = "ENV";
    injected.type = SilEventType::FaultInjected;
    injected.severity = EventSeverity::Info;
    injected.detail = fault_type_name(scenario.fault_type);
    ctx.trace.record(injected);

    const auto record_typed = [&ctx, &scenario](SilEventType type, double value) {
        SilEvent event;

        event.timestamp = ctx.time;
        event.source = "ENV";
        event.type = type;
        event.severity = EventSeverity::Warning;
        event.value = value;
        event.has_value = true;
        ctx.trace.record(event);
    };

    if (scenario.fault_type == FaultType::SensorFault) {
        record_typed(SilEventType::SensorFault, scenario.parameters.corrupted_altitude_m);
    }
    if (scenario.fault_type == FaultType::ActuatorDegradation) {
        record_typed(SilEventType::ActuatorFault, scenario.parameters.efficiency);
    }
}

}