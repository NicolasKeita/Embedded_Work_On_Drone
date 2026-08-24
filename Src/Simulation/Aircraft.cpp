/*
Filename: Src/Simulation/Aircraft.cpp
Description: Integration of the simplified Heliblade-like flight physics (actuators, attitude, translation).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Aircraft;

import std;

namespace
{
    // Parametres vehicule.
    constexpr double kGravityMps2 = 9.81;
    constexpr double kMassKg = 1.2;

    // Gains aerodynamiques / geometriques.
    constexpr double kLiftCoeff = 1.77e-5;     // Lift = kLiftCoeff * rpm^2 [N]
    constexpr double kPitchTargetGain = 0.01;  // rad par degre de consigne moyenne des servos
    constexpr double kRollTargetGain = 0.01;   // rad par degre de differenciel des servos
    constexpr double kPitchAccelGain = 9.81;   // m/s^2 par radian de pitch
    constexpr double kRollAccelGain = 9.81;    // m/s^2 par radian de roll

    // Constante de temps du lissage d'attitude du premier ordre (s).
    constexpr double kAttitudeTauS = 0.25;

    // Limites physiques des actionneurs.
    constexpr double kMinRpm = 0.0;
    constexpr double kMaxRpm = 12000.0;
    constexpr double kMinServoDeg = -30.0;
    constexpr double kMaxServoDeg = 30.0;

    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

void Aircraft::update_actuators()
{
    // Prototype : actionneurs parfaits (etat effectif = consigne).
    // Point d'extension : ajouter ici une dynamique du premier ordre
    // (inertie moteur / servo) entre command_ et state_.
    state_.actual_rpm = Clamp(command_.wing_rpm, kMinRpm, kMaxRpm);
    state_.actual_left_servo = Clamp(command_.left_servo_angle, kMinServoDeg, kMaxServoDeg);
    state_.actual_right_servo = Clamp(command_.right_servo_angle, kMinServoDeg, kMaxServoDeg);
}

void Aircraft::update_attitude(double dt)
{
    const double pitchTarget =
        kPitchTargetGain * (state_.actual_left_servo + state_.actual_right_servo) / 2.0;
    const double rollTarget =
        kRollTargetGain * (state_.actual_left_servo - state_.actual_right_servo);

    // Dynamique lissee du premier ordre : pas de saut instantane d'attitude.
    state_.pitch_rate = (pitchTarget - state_.pitch) / kAttitudeTauS;
    state_.roll_rate = (rollTarget - state_.roll) / kAttitudeTauS;

    state_.pitch += state_.pitch_rate * dt;
    state_.roll += state_.roll_rate * dt;
}

void Aircraft::update_translation(double dt)
{
    // Bilan des forces verticales.
    const double lift = kLiftCoeff * state_.actual_rpm * state_.actual_rpm;
    const double weight = kMassKg * kGravityMps2;
    const double az = (lift - weight) / kMassKg;

    // Accelerations horizontales induites par l'attitude.
    const double ax = kPitchAccelGain * state_.pitch;
    const double ay = kRollAccelGain * state_.roll;

    // Integration d'Euler explicite : force -> acceleration -> vitesse -> position.
    state_.vz += az * dt;
    state_.vx += ax * dt;
    state_.vy += ay * dt;

    state_.x += state_.vx * dt;
    state_.y += state_.vy * dt;
    state_.z += state_.vz * dt;

    // Contact sol : blocage a z = 0 tant que la vitesse verticale est descendante.
    if (state_.z <= 0.0) {
        state_.z = 0.0;
        if (state_.vz < 0.0) {
            state_.vz = 0.0;
        }
    }
}

void Aircraft::update(double dt)
{
    update_actuators();
    update_attitude(dt);
    update_translation(dt);
}

void Aircraft::set_command(const ControlCommand& cmd)
{
    command_ = cmd;
}

const AircraftState& Aircraft::state() const
{
    return state_;
}

double Aircraft::hover_rpm() const
{
    return std::sqrt(kMassKg * kGravityMps2 / kLiftCoeff);
}
