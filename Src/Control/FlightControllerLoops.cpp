/*
Filename: Src/Control/FlightControllerLoops.cpp
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
Boucles de position horizontale en cascade (boucle externe) : chaque erreur de
position produit une consigne d'inclinaison saturee a max_tilt_deg ; le terme
derive amortit la vitesse horizontale pour eviter le depassement typique d'un
systeme a double integrateur.
*/
TiltTargets FlightController::updatePositionControl(const TargetState& t,
                                                    const AircraftState& a, double dt)
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
Boucle d'attitude en cascade (boucle interne) : suit chaque consigne
d'inclinaison avec un gain proportionnel exprime en degres servo equivalents
(1 degre servo = kServoRadPerDeg rad d'inclinaison). Le melange respecte le
modele physique : moyenne des servos -> tangage, difference -> roulis.
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
Commande de tenue de station : cascade complete, altitude -> RPM et position ->
attitude -> servos, recombinees en une seule commande actionneurs.
*/
ControlCommand FlightController::station_keeping_command(const TargetState& t,
                                                         const AircraftState& a, double dt)
{
    ControlCommand cmd;
    cmd.wing_rpm = updateAltitudeControl(t.z, a.z, dt);

    const ServoMix servos = updateAttitudeControl(updatePositionControl(t, a, dt), a);
    cmd.left_servo_angle = servos.left_deg;
    cmd.right_servo_angle = servos.right_deg;

    return cmd;
}

/*
Zone cible : l'appareil est considere en station des que les trois ecarts sont
inferieurs aux tolerances configurees.
*/
bool FlightController::inside_target_zone(const TargetState& t, const AircraftState& a) const
{
    return std::abs(t.z - a.z) <= config_.altitude_tolerance_m
        && std::abs(t.x - a.x) <= config_.position_tolerance_m
        && std::abs(t.y - a.y) <= config_.position_tolerance_m;
}}
