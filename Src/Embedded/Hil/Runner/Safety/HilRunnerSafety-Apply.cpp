/*
Filename: Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Apply.cpp
Description: Application of the HIL safety mode to the actuator command : SAFE_MODE
controlled descent, COMPENSATED thrust margin, actuator efficiency and physics update.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerSafety;

import std;

import Aircraft;
import HalTypes;
import HilConfig;
import HilRunnerContext;
import SafetyManager;

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

/*
Applies the returned actuator command to the aircraft: holds the last command on a
missed response, engages the SAFE_MODE controlled descent or the COMPENSATED thrust
margin, then scales by the actuator efficiency and integrates the physics. The aircraft
state evolves from actuator commands and configured wind disturbances.
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
    const std::float64_t wind = sim::hil::wind_factor(cfg, ctx.time);
    ctx.aircraft.set_wind(cfg.wind_x_mps * wind, cfg.wind_y_mps * wind);
    ctx.aircraft.update(cfg.dt_s);
}

}
