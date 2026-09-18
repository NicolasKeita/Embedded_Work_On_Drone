/*
Filename: Src/Simulation/Aircraft-Physics.cpp
Description: Attitude dynamics and translation integration of the Aircraft physics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Aircraft;

import std;

import PhysicsDispersion;

namespace
{
    constexpr std::float64_t kGravityMps2 = 9.81;
    constexpr std::float64_t kBaseMassKg = 1.2;

    constexpr std::float64_t kLiftCoeff = 1.77e-5;
    constexpr std::float64_t kPitchTargetGainRadPerDeg = 0.01;
    constexpr std::float64_t kRollTargetGainRadPerDeg = 0.01;
    constexpr std::float64_t kPitchAccelGainMps2PerRad = 9.81;
    constexpr std::float64_t kRollAccelGainMps2PerRad = 9.81;

    constexpr std::float64_t kAttitudeTauS = 0.25;
}

/* Sets the deterministic horizontal wind used by the translation model. */
void Aircraft::set_wind(std::float64_t x_mps, std::float64_t y_mps) noexcept
{
    wind_x_mps_ = x_mps;
    wind_y_mps_ = y_mps;
}

/* Smooth first-order attitude dynamics with time constant kAttitudeTauS. */
void Aircraft::update_attitude(std::float64_t dt)
{
    const std::float64_t pitchTarget =
        kPitchTargetGainRadPerDeg * (state_.actual_left_servo + state_.actual_right_servo) / 2.0;
    const std::float64_t rollTarget = kRollTargetGainRadPerDeg * (state_.actual_left_servo - state_.actual_right_servo);

    state_.pitch_rate = (pitchTarget - state_.pitch) / kAttitudeTauS;
    state_.roll_rate = (rollTarget - state_.roll) / kAttitudeTauS;

    state_.pitch += state_.pitch_rate * dt;
    state_.roll += state_.roll_rate * dt;
}

/*
Translation: vertical force balance (lift vs weight), attitude-induced horizontal accelerations
and a simplified linear wind disturbance (0.08 / s), then explicit Euler integration. Ground
contact locked at z = 0.
*/
void Aircraft::update_translation(std::float64_t dt)
{
    const std::float64_t current_mass = kBaseMassKg * (1.0 + dispersion_.mass_variation);
    const std::float64_t lift = kLiftCoeff * state_.actual_rpm * state_.actual_rpm;
    const std::float64_t weight = current_mass * kGravityMps2;
    const std::float64_t az = (lift - weight) / current_mass;
    const std::float64_t wind_gain = state_.z > 0.0 ? 0.08 : 0.0;
    const std::float64_t ax = kPitchAccelGainMps2PerRad * state_.pitch + wind_gain * wind_x_mps_;
    const std::float64_t ay = kRollAccelGainMps2PerRad * state_.roll + wind_gain * wind_y_mps_;

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
