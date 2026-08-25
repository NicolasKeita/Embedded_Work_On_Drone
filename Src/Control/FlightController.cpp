/*
Filename: Src/Control/FlightController.cpp
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
    altitude_pid_.integral_limit =
        config.max_integral_rpm / std::max(config.ki_altitude, 1e-9);
    altitude_pid_.integral_error_band = kAltitudeIntegralErrorBandM;

    x_position_pid_.kp = config.kp_position;
    x_position_pid_.kd = config.kd_position;
    y_position_pid_.kp = config.kp_position;
    y_position_pid_.kd = config.kd_position;
}

/*
Retourne l'etat courant de la machine a etats de mission.
*/
MissionState FlightController::state() const
{
    return mission_state_;
}

/*
Reinitialise l'etat des PIDs (integrale et memoire de derivee) sans toucher aux
gains, afin d'eviter tout coup de derivee ou windup residuel au changement de
phase de mission.
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
Reinitialise les PIDs puis bascule la machine a etats en CLIMB.
*/
void FlightController::enter_climb()
{
    reset_pids();
    mission_state_ = MissionState::CLIMB;
}

/*
Pas PID generique : derivee numerique de l'erreur, integrale bridee (anti-
windup) qui n'accumule qu'a proximite de la cible (bande configuree). La
structure reste identique pour un regulateur P pur (ki = kd = 0), PI ou PID.
*/
double FlightController::AxisPidStep(AxisPid& pid, double error, double dt)
{
    if (!pid.primed) {
        pid.previous_error = error;
        pid.primed = true;
    }

    const double errorDerivative = (error - pid.previous_error) / dt;
    pid.previous_error = error;

    const bool inBand =
        pid.integral_error_band == 0.0 || std::abs(error) <= pid.integral_error_band;
    if (inBand) {
        pid.integral =
            Clamp(pid.integral + error * dt, -pid.integral_limit, pid.integral_limit);
    }

    return pid.kp * error + pid.ki * pid.integral + pid.kd * errorDerivative;
}

/*
Boucle d'altitude : erreur de position verticale convertie en correction RPM
autour du point d'equilibre hover_rpm (la portance y compense exactement le
poids). Kp agit sur l'erreur, Kd amortit la vitesse verticale (evite les
oscillations du P pur sur ce systeme a double integrateur), Ki elimine
l'erreur statique residuelle. Correction saturee entre min_rpm et max_rpm.
*/
double FlightController::updateAltitudeControl(double target_z, double actual_z, double dt)
{
    const double correction = AxisPidStep(altitude_pid_, target_z - actual_z, dt);
    return Clamp(config_.hover_rpm + correction, config_.min_rpm, config_.max_rpm);
}}
