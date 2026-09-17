/*
Filename: Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Apply.cpp
Description: Application of the HIL safety mode to the actuator command : SAFE_MODE
position hold, COMPENSATED thrust margin, actuator efficiency and physics update.

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
Applies the returned actuator command to the aircraft: SAFE_MODE freezes the
aircraft at its last position, COMPENSATED applies the thrust margin, then the
actuator efficiency scales the command and the physics integrates the state.
The aircraft state evolves from actuator commands and configured wind
disturbances.
*/
void apply_actuators(HilRunContext& ctx)
{
    const HilConfig& cfg = ctx.config;

    if (ctx.safety.mode() == sim::safety::SafetyMode::SAFE_MODE) {
        ctx.commanded_rpm = ctx.last_effective_rpm;
        return;
    }

    ControlCommand effective = ctx.this_received ? to_control_command(ctx.actuator_cmd) : ctx.command;

    if (ctx.safety.mode() == sim::safety::SafetyMode::COMPENSATED) {
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
