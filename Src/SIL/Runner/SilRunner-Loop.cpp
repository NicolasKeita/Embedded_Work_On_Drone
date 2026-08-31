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
    record_watchdog_and_detection(ctx, report);
    record_health_transition(ctx, report.state);
    record_safety_transitions(ctx);
    record_recovery_end(ctx, report.state);

    ctx.result.degraded_reached = ctx.result.degraded_reached || report.state == HealthState::DEGRADED;
    ctx.result.compensated_reached =
        ctx.result.compensated_reached || ctx.safety.mode() == SafetyMode::COMPENSATED;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || ctx.safety.mode() == SafetyMode::SAFE_MODE;
    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
    ctx.previous_health = report.state;
}

/*
Records one health state transition with its previous/new states.
*/
void SILRunner::record_health_transition(RunContext& ctx, sim::safety::HealthState current)
{
    if (current == ctx.previous_health) {
        return;
    }

    SilEvent transition;

    transition.timestamp = ctx.time;
    transition.source = "FC2";
    transition.type = SilEventType::SafetyStateTransition;
    transition.severity = EventSeverity::Info;
    transition.previous_state = health_state_name(ctx.previous_health);
    transition.new_state = health_state_name(current);
    transition.reason = current == HealthState::HEALTHY ? "all flags clear" : "fault flags raised";
    ctx.trace.record(transition);
}

/*
Records the recovery completion when health returns to HEALTHY after a fault.
*/
void SILRunner::record_recovery_end(RunContext& ctx, sim::safety::HealthState current)
{
    if (ctx.result.detection_time < 0.0 || current != HealthState::HEALTHY || ctx.result.recovery_time >= 0.0) {
        return;
    }
    ctx.result.recovery_time = ctx.time;
    ctx.result.recovery_successful = true;

    SilEvent recovery_end;
    recovery_end.timestamp = ctx.time;
    recovery_end.source = "FC2";
    recovery_end.type = SilEventType::RecoveryEnd;
    recovery_end.severity = EventSeverity::Info;
    recovery_end.reason = "health restored";

    SilEvent watchdog_recovery = recovery_end;
    watchdog_recovery.type = SilEventType::WatchdogRecovery;
    ctx.trace.record(recovery_end);
    ctx.trace.record(watchdog_recovery);
}

/*
Actuator step: FC1 command replaced by a controlled descent in SAFE_MODE and
scaled by the thrust margin in COMPENSATED mode, then the environment applies
the actuator efficiency and the physics integrates the aircraft state.
*/
void SILRunner::apply_actuators(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;
    ControlCommand effective = ctx.command;

    if (ctx.safety.mode() == SafetyMode::SAFE_MODE) {
        ctx.safe_rpm = std::max(0.0, ctx.last_effective_rpm - cfg.safe_descent_rpm_rate * cfg.dt);
        effective.wing_rpm = ctx.safe_rpm;
        effective.left_servo_angle = 0.0;
        effective.right_servo_angle = 0.0;
    }
    else if (ctx.safety.mode() == SafetyMode::COMPENSATED) {
        effective.wing_rpm *= ctx.safety_command.thrust_margin;
    }
    ctx.commanded_rpm = effective.wing_rpm;
    ctx.last_effective_rpm = effective.wing_rpm;
    effective.wing_rpm *= ctx.env.actuator_efficiency;
    ctx.aircraft.set_command(effective);
    ctx.aircraft.update(cfg.dt);
}

}