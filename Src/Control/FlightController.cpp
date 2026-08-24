/*
Filename: Src/Control/FlightController.cpp
Description: Implementation of the velocity-level flight control cascade (RPM and servo mixing).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightController;

import std;

import Aircraft;

namespace
{
    constexpr double kMinRpm = 0.0;
    constexpr double kMinServoDeg = -30.0;
    constexpr double kMaxServoDeg = 30.0;

    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

namespace sim::control {

FlightController::FlightController(ControllerConfig config) : config_(config)
{
}

/*
Boucle verticale : correction proportionnelle de l'erreur de vitesse verticale
autour du stationnaire. La portance etant quadratique en RPM, ce gain n'est
valide qu'autour du point de stationnaire (lineairisation locale).
Point d'extension : ajouter une integrale (PI) ou un terme derive.
*/
double FlightController::vertical_rpm(const AircraftState&    state,
                                      const VelocitySetpoint& setpoint) const
{
    const double vzError = setpoint.target_vz - state.vz;
    const double rpm = config_.hover_rpm + config_.kp_vertical * vzError;

    return Clamp(rpm, kMinRpm, std::numeric_limits<double>::max());
}

/*
Boucles horizontales : chaque erreur de vitesse est convertie en angle de
consigne (degres), saturee a max_tilt_deg, puis melangee sur les servos selon
le modele physique (cf. Aircraft::update_attitude) :
  moyenne   (left + right) / 2 -> tangage (donc vitesse X)
  differentiel left - right    -> roulis  (donc vitesse Y)
*/
ControlCommand FlightController::horizontal_servos(const AircraftState&    state,
                                                   const VelocitySetpoint& setpoint) const
{
    const double tiltLimit = config_.max_tilt_deg;

    const double pitchDeg =
        Clamp(config_.kp_horizontal * (setpoint.target_vx - state.vx),
              -tiltLimit, tiltLimit);
    const double rollDeg =
        Clamp(config_.kp_horizontal * (setpoint.target_vy - state.vy),
              -tiltLimit, tiltLimit);

    ControlCommand cmd;
    cmd.wing_rpm = config_.hover_rpm;
    cmd.left_servo_angle =
        Clamp(pitchDeg + rollDeg, kMinServoDeg, kMaxServoDeg);
    cmd.right_servo_angle =
        Clamp(pitchDeg - rollDeg, kMinServoDeg, kMaxServoDeg);

    return cmd;
}

/*
Cascade complete : la boucle verticale fixe le RPM, les boucles horizontales
fixent le melange servo ; les deux sont recombinees en une seule commande.
*/
ControlCommand FlightController::compute_command(const AircraftState&    state,
                                                 const VelocitySetpoint& setpoint) const
{
    ControlCommand cmd = horizontal_servos(state, setpoint);
    cmd.wing_rpm = vertical_rpm(state, setpoint);

    return cmd;
}

} // namespace sim::control
