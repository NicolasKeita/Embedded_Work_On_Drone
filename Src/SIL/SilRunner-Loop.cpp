/*
Filename: Src/SIL/SilRunner-Loop.cpp
Description: SIL loop step : monitoring, actuators, metrics and result finalization.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import Aircraft;
import CommsBus;
import FaultInjectors;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::HealthState;
using sim::safety::HealthReport;
using sim::safety::SafetyMode;
using sim::safety::fault_domain_name;

void SILRunner::update_monitoring(RunContext& ctx)
{
    const HealthReport report = ctx.health.evaluate(ctx.time, ctx.comms, ctx.telemetry, ctx.commanded_rpm);

    ctx.safety_command = ctx.safety.update(ctx.time, report);
    const double detection = report.first_detection_time();
    if (detection >= 0.0 && ctx.result.detection_time < 0.0) {
        ctx.result.detection_time = detection;
        ctx.result.first_fault_domain = report.first_fault_domain();
        ctx.result.fault_detected = true;
    }
    if (ctx.result.detection_time >= 0.0 && report.state == HealthState::HEALTHY && ctx.result.recovery_time < 0.0) {
        ctx.result.recovery_time = ctx.time;
    }
    ctx.result.degraded_reached = ctx.result.degraded_reached || report.state == HealthState::DEGRADED;
    ctx.result.compensated_reached = ctx.result.compensated_reached || ctx.safety.mode() == SafetyMode::COMPENSATED;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || ctx.safety.mode() == SafetyMode::SAFE_MODE;
    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
}

/*
Actuator step: FC1 command (or descent command kept in SAFE_MODE), generic
thrust margin in COMPENSATED mode, then actuator efficiency applied by the
environment before physics integration.
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

void SILRunner::update_metrics(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;
    const AircraftState& state = ctx.aircraft.state();
    const double position_error = std::hypot(state.x - cfg.target.x, state.y - cfg.target.y);
    const double altitude_error = std::abs(state.z - cfg.target.z);

    ctx.result.max_position_error_m = std::max(ctx.result.max_position_error_m, position_error);
    ctx.result.max_altitude_error_m = std::max(ctx.result.max_altitude_error_m, altitude_error);
    ctx.result.final_altitude_m = state.z;
    ctx.result.final_state = ctx.fc1.state();
    if (!ctx.result.mission_aborted && ctx.safety.mode() == SafetyMode::SAFE_MODE) {
        ctx.result.mission_aborted = true;
        ctx.result.failure_reason = std::string{"Safe mode engage : "}
            + std::string{fault_domain_name(ctx.result.first_fault_domain)};
    }
}

void SILRunner::finalize(RunContext& ctx)
{
    SimulationResult& result = ctx.result;

    if (result.fault_injected_time >= 0.0 && result.detection_time >= 0.0) {
        result.detection_latency = result.detection_time - result.fault_injected_time;
    }
    if (result.detection_time >= 0.0 && ctx.safety.response_time() >= 0.0) {
        result.response_latency = ctx.safety.response_time() - result.detection_time;
    }
    result.mission_success = result.final_state == sim::control::MissionState::COMPLETE && !result.mission_aborted;
}

}
