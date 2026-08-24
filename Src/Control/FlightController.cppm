/*
Filename: Src/Control/FlightController.cppm
Description: Public interface of the velocity-level flight controller producing actuator commands.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightController;

import std;

import Aircraft;

export namespace sim::control {

// Gains et limites du regulateur ; hover_rpm doit etre fourni par l'aeronef de reference.
struct ControllerConfig {
    double hover_rpm{0.0};      // Stationnaire theorique (Aircraft::hover_rpm()).
    double kp_vertical{120.0};  // tr/min par (m/s) d'erreur verticale.
    double kp_horizontal{2.0};  // degres par (m/s) d'erreur horizontale.
    double max_tilt_deg{15.0};  // Consigne d'inclinaison maximale envoyee aux servos.
};

// Consigne de vol exprimee en vitesses cibles (m/s).
struct VelocitySetpoint {
    double target_vz = 0.0;
    double target_vx = 0.0;
    double target_vy = 0.0;
};

class FlightController {
public:
    explicit FlightController(ControllerConfig config);

    /*
    Calcule la commande actionneurs (RPM aile + angles servos gauche/droit)
    a partir de l'etat mesuré et de la consigne de vitesse. Methode pure :
    aucun etat interne, compatible avec un appel periodique deterministe.
    */
    [[nodiscard]] ControlCommand compute_command(const AircraftState& state,
                                                 const VelocitySetpoint& setpoint) const;

private:
    [[nodiscard]] double vertical_rpm(const AircraftState& state,
                                      const VelocitySetpoint& setpoint) const;
    [[nodiscard]] ControlCommand horizontal_servos(const AircraftState& state,
                                                   const VelocitySetpoint& setpoint) const;

    ControllerConfig config_;
};

} // namespace sim::control
