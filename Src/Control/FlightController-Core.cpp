/*
Filename: Src/Control/FlightController-Core.cpp
Description: Core of the autonomous flight controller : PID machinery and mission state helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace
{
    constexpr double kAltitudeIntegralErrorBandM = 5.0;

    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

namespace sim::control {

FlightController::FlightController(ControllerConfig config) : config_(config)
{
    altitude_pid_.kp = config.kp_altitude;
    altitude_pid_.ki = config.ki_altitude;
    altitude_pid_.kd = config.kd_altitude;
    altitude_pid_.integral_limit = config.max_integral_rpm / std::max(config.ki_altitude, 1e-9);
    altitude_pid_.integral_error_band = kAltitudeIntegralErrorBandM;

    x_position_pid_.kp = config.kp_position;
    x_position_pid_.kd = config.kd_position;
    y_position_pid_.kp = config.kp_position;
    y_position_pid_.kd = config.kd_position;
}

/*
Returns the current state of the mission state machine.
*/
MissionState FlightController::state() const
{
    return mission_state_;
}

/*
Resets the PID states (integral and derivative memory) without touching the
gains, to avoid any derivative kick or residual windup when switching mission
phase.
*/
void FlightController::reset_pids()
{
    altitude_pid_.integral = 0.0;
    altitude_pid_.previous_error = 0.0;
    altitude_pid_.primed = false;
    x_position_pid_.integral = 0.0;
    x_position_pid_.previous_error = 0.0;
    x_position_pid_.primed = false;
    y_position_pid_.integral = 0.0;
    y_position_pid_.previous_error = 0.0;
    y_position_pid_.primed = false;
}

/*
Resets the PIDs then switches the mission state machine to CLIMB.
*/
void FlightController::enter_climb()
{
    reset_pids();
    mission_state_ = MissionState::CLIMB;
}

/*
Generic PID step: numerical derivative of the error, clamped integral (anti-
windup) accumulating only near the target (configured band). The structure
remains identical for a pure P controller (ki = kd = 0), PI or PID.
*/
double FlightController::AxisPidStep(AxisPid& pid, double error, double dt)
{
    if (!pid.primed) {
        pid.previous_error = error;
        pid.primed = true;
    }

    const double errorDerivative = (error - pid.previous_error) / dt;
    pid.previous_error = error;

    const bool inBand = pid.integral_error_band == 0.0 || std::abs(error) <= pid.integral_error_band;
    if (inBand) {
        pid.integral = Clamp(pid.integral + error * dt, -pid.integral_limit, pid.integral_limit);
    }

    return pid.kp * error + pid.ki * pid.integral + pid.kd * errorDerivative;
}

/*
Altitude loop: vertical position error converted into an RPM correction around
the equilibrium point hover_rpm (where lift exactly compensates weight). Kp acts
on the error, Kd damps the vertical velocity (avoids the pure P oscillations on
this double-integrator system), Ki removes the residual steady-state error.
Correction saturated between min_rpm and max_rpm.
*/
double FlightController::updateAltitudeControl(double target_z, double actual_z, double dt)
{
    const double correction = AxisPidStep(altitude_pid_, target_z - actual_z, dt);

    return Clamp(config_.hover_rpm + correction, config_.min_rpm, config_.max_rpm);
}}
