/*
Filename: Src/SIL/Runner/SilRunner-Loop.cpp
Description: Monitoring step with health transitions, recovery recording and actuator application.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import Aircraft;
import CommsBus;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilRunnerEvents;
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::HealthState;
using sim::safety::HealthReport;
using sim::safety::SafetyMode;

/*
Monitoring step: health evaluation, safety update and event recording.
*/
void SILRunner::update_monitoring(RunContext& ctx)
{
    const HealthReport report = ctx.health.evaluate(ctx.time, ctx.comms, ctx.telemetry, ctx.commanded_rpm);

    ctx.safety_command = ctx.safety.update(ctx.time, report);
    record_supervision_and_detection(ctx, report);
    record_health_transition(ctx, report.state);
    record_safety_transitions(ctx);
    record_recovery_end(ctx, report.state);

    ctx.result.degraded_reached = ctx.result.degraded_reached || report.state == HealthState::DEGRADED;
    ctx.result.compensated_reached = ctx.result.compensated_reached || ctx.safety.mode() == SafetyMode::COMPENSATED;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || ctx.safety.mode() == SafetyMode::SAFE_MODE;
    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
    ctx.previous_health = report.state;
}

/*
Actuator step: the FC1 command is held at the last applied value in SAFE_MODE
and scaled by the thrust margin in COMPENSATED mode, then the environment
applies the actuator efficiency and the physics integrates the aircraft state.
*/
void SILRunner::apply_actuators(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;

    if (ctx.safety.mode() == SafetyMode::SAFE_MODE) {
        ctx.commanded_rpm = ctx.last_effective_rpm;
        return;
    }

    ControlCommand effective = ctx.command;

    if (ctx.safety.mode() == SafetyMode::COMPENSATED) {
        effective.wing_rpm *= ctx.safety_command.thrust_margin;
    }
    ctx.commanded_rpm = effective.wing_rpm;
    ctx.last_effective_rpm = effective.wing_rpm;
    effective.wing_rpm *= ctx.env.actuator_efficiency;
    ctx.aircraft.set_command(effective);
    ctx.aircraft.update(cfg.dt);
}

}
