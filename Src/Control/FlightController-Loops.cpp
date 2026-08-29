/*
Filename: Src/Control/FlightController-Loops.cpp
Description: Cascaded horizontal position and attitude control loops of the flight controller.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace
{
    constexpr double kMinServoDeg = -30.0;
    constexpr double kMaxServoDeg = 30.0;

    constexpr double kServoRadPerDeg = 0.01;

    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }

    double DegToRad(double valueDeg)
    {
        return valueDeg * std::numbers::pi / 180.0;
    }
}

namespace sim::control {

/*
Cascaded horizontal position loops (outer loop): each position error produces a
tilt setpoint saturated at max_tilt_deg; the derivative term damps the
horizontal velocity to avoid the typical overshoot of a double-integrator
system.
*/
TiltTargets FlightController::updatePositionControl(const TargetState&   t,
                                                    const AircraftState& a,
                                                    double               dt)
{
    TiltTargets targets;
    targets.pitch_deg =
        Clamp(AxisPidStep(x_position_pid_, t.x - a.x, dt),
              -config_.max_tilt_deg, config_.max_tilt_deg);
    targets.roll_deg =
        Clamp(AxisPidStep(y_position_pid_, t.y - a.y, dt),
              -config_.max_tilt_deg, config_.max_tilt_deg);
    return targets;
}

/*
Cascaded attitude loop (inner loop): tracks each tilt setpoint with a
proportional gain expressed in equivalent servo degrees (1 servo degree =
kServoRadPerDeg rad of tilt). The mix follows the physical model: servo mean ->
pitch, difference -> roll.
*/
ServoMix FlightController::updateAttitudeControl(TiltTargets tilt, const AircraftState& s) const
{
    const double pitchErrorDeg =
        (DegToRad(tilt.pitch_deg) - s.pitch) / kServoRadPerDeg;
    const double rollErrorDeg =
        (DegToRad(tilt.roll_deg) - s.roll) / kServoRadPerDeg;

    const double meanServoDeg = tilt.pitch_deg + config_.kp_attitude * pitchErrorDeg;
    const double halfRollServoDeg =
        (tilt.roll_deg + config_.kp_attitude * rollErrorDeg) / 2.0;

    return {Clamp(meanServoDeg + halfRollServoDeg, kMinServoDeg, kMaxServoDeg),
            Clamp(meanServoDeg - halfRollServoDeg, kMinServoDeg, kMaxServoDeg)};
}

/*
Station keeping command: full cascade, altitude -> RPM and position ->
attitude -> servos, recombined into a single actuator command.
*/
ControlCommand FlightController::station_keeping_command(const TargetState&   t,
                                                         const AircraftState& a,
                                                         double               dt)
{
    ControlCommand cmd;
    cmd.wing_rpm = updateAltitudeControl(t.z, a.z, dt);

    const ServoMix servos = updateAttitudeControl(updatePositionControl(t, a, dt), a);
    cmd.left_servo_angle = servos.left_deg;
    cmd.right_servo_angle = servos.right_deg;

    return cmd;
}

/*
Target zone: the vehicle is considered in station as soon as all three errors
are below the configured tolerances.
*/
bool FlightController::inside_target_zone(const TargetState& t, const AircraftState& a) const
{
    return std::abs(t.z - a.z) <= config_.altitude_tolerance_m
        && std::abs(t.x - a.x) <= config_.position_tolerance_m
        && std::abs(t.y - a.y) <= config_.position_tolerance_m;
}}
