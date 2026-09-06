/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Apply.cpp
Description: Host-side health/safety evaluation (the FC2 monitoring role, reusing the SIL
HealthMonitor/SafetyManager) and application of the returned actuator command to the
aircraft (SAFE_MODE/COMPENSATED overrides mirroring the SIL actuator step).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import Aircraft;
import CommsBus;
import HalTypes;
import HealthMonitor;
import HilConfig;
import HilRunnerContext;
import HilRunnerEvents;
import SafetyManager;
import SilTypes;

namespace sim::hil {

namespace {
    constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;

    ControlCommand to_control_command(const FlightCore::HAL::ActuatorCommands& cmds) noexcept
    {
        return ControlCommand{
            .wing_rpm = cmds.wing_rpm_cmd,
            .left_servo_angle = cmds.left_servo_rad * kDegreesPerRadian,
            .right_servo_angle = cmds.right_servo_rad * kDegreesPerRadian,
        };
    }
}

void update_health_and_safety(HilRunContext& ctx)
{
    const sim::safety::HealthReport report =
        ctx.health.evaluate(ctx.time, ctx.comms, ctx.sensors, ctx.commanded_rpm);

    ctx.safety_command = ctx.safety.update(ctx.time, report);
    record_detection(ctx, report);
    record_safety_transitions(ctx);

    const bool degraded = report.state == sim::safety::HealthState::DEGRADED;
    const bool compensated = ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED;
    const bool safe_mode = ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE;

    ctx.result.final_health = report.state;
    ctx.result.final_safety_mode = ctx.safety.mode();
    ctx.result.degraded_reached = ctx.result.degraded_reached || degraded;
    ctx.result.compensated_reached = ctx.result.compensated_reached || compensated;
    ctx.result.safe_mode_reached = ctx.result.safe_mode_reached || safe_mode;
}

/*
Applies the returned actuator command to the aircraft: holds the last command on a
missed response, engages the SAFE_MODE controlled descent or the COMPENSATED thrust
margin, then scales by the actuator efficiency and integrates the physics. The aircraft
state evolves only from actuator commands — it is never replayed.
*/
void apply_actuators(HilRunContext& ctx)
{
    const HilConfig& cfg = ctx.config;
    ControlCommand   effective = ctx.this_received ? to_control_command(ctx.actuator_cmd) : ctx.command;

    if (ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE) {
        ctx.safe_rpm = std::max(std::float64_t{0.0}, ctx.last_effective_rpm - cfg.safe_descent_rpm_rate * cfg.dt_s);
        effective.wing_rpm = ctx.safe_rpm;
        effective.left_servo_angle = 0.0;
        effective.right_servo_angle = 0.0;
    }
    else if (ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED) {
        effective.wing_rpm *= ctx.safety_command.thrust_margin;
    }
    ctx.command = effective;
    ctx.commanded_rpm = effective.wing_rpm;
    ctx.last_effective_rpm = effective.wing_rpm;
    effective.wing_rpm *= ctx.env.actuator_efficiency;
    ctx.aircraft.set_command(effective);
    ctx.aircraft.update(cfg.dt_s);
}

}
