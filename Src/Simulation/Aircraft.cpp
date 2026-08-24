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
    constexpr double kGravityMps2 = 9.81;
    constexpr double kMassKg = 1.2;

    constexpr double kLiftCoeff = 1.77e-5;
    constexpr double kPitchTargetGainRadPerDeg = 0.01;
    constexpr double kRollTargetGainRadPerDeg = 0.01;
    constexpr double kPitchAccelGainMps2PerRad = 9.81;
    constexpr double kRollAccelGainMps2PerRad = 9.81;

    constexpr double kAttitudeTauS = 0.25;

    constexpr double kMinRpm = 0.0;
    constexpr double kMaxRpm = 12000.0;
    constexpr double kMinServoDeg = -30.0;
    constexpr double kMaxServoDeg = 30.0;

    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

/*
Actionneurs parfaits (etat effectif = consigne).
Point d'extension : ajouter ici une dynamique du premier ordre
(inertie moteur / servo) entre command_ et state_.
*/
void Aircraft::update_actuators()
{
    state_.actual_rpm = Clamp(command_.wing_rpm, kMinRpm, kMaxRpm);
    state_.actual_left_servo = Clamp(command_.left_servo_angle, kMinServoDeg, kMaxServoDeg);
    state_.actual_right_servo = Clamp(command_.right_servo_angle, kMinServoDeg, kMaxServoDeg);
}

/*
Dynamique lissee du premier ordre : pas de saut instantane d'attitude,
la consigne moyenne (pitch) et le differentiel (roll) des servos sont suivis
avec une constante de temps kAttitudeTauS.
*/
void Aircraft::update_attitude(double dt)
{
    const double pitchTarget =
        kPitchTargetGainRadPerDeg * (state_.actual_left_servo + state_.actual_right_servo) / 2.0;
    const double rollTarget =
        kRollTargetGainRadPerDeg * (state_.actual_left_servo - state_.actual_right_servo);

    state_.pitch_rate = (pitchTarget - state_.pitch) / kAttitudeTauS;
    state_.roll_rate = (rollTarget - state_.roll) / kAttitudeTauS;

    state_.pitch += state_.pitch_rate * dt;
    state_.roll += state_.roll_rate * dt;
}

/*
Translation : bilan des forces verticales (portance vs poids), accelerations
horizontales induites par l'attitude, puis integration d'Euler explicite :
force -> acceleration -> vitesse -> position.
Contact sol : blocage a z = 0 tant que la vitesse verticale est descendante.
*/
void Aircraft::update_translation(double dt)
{
    const double lift = kLiftCoeff * state_.actual_rpm * state_.actual_rpm;
    const double weight = kMassKg * kGravityMps2;
    const double az = (lift - weight) / kMassKg;

    const double ax = kPitchAccelGainMps2PerRad * state_.pitch;
    const double ay = kRollAccelGainMps2PerRad * state_.roll;

    state_.vz += az * dt;
    state_.vx += ax * dt;
    state_.vy += ay * dt;

    state_.x += state_.vx * dt;
    state_.y += state_.vy * dt;
    state_.z += state_.vz * dt;

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
