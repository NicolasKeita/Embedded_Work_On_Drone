/*
Filename: Src/Simulation/Aircraft-Physics.cpp
Description: Attitude dynamics and translation integration of the Aircraft physics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Aircraft;

import std;

import PhysicsDispersion;

/* Sets the deterministic horizontal wind used by the translation model. */
void Aircraft::set_wind(std::float64_t x_mps, std::float64_t y_mps) noexcept
{
    wind_x_mps_ = x_mps;
    wind_y_mps_ = y_mps;
}

/* Smooth first-order attitude dynamics with the configured time constant. */
void Aircraft::update_attitude(std::float64_t dt)
{
    const std::float64_t pitchTarget =
        parameters_.pitch_target_gain_rad_per_deg * (state_.actual_left_servo + state_.actual_right_servo) / 2.0;
    const std::float64_t rollTarget = parameters_.roll_target_gain_rad_per_deg * (state_.actual_left_servo - state_.actual_right_servo);

    state_.pitch_rate = (pitchTarget - state_.pitch) / parameters_.attitude_time_constant_s;
    state_.roll_rate = (rollTarget - state_.roll) / parameters_.attitude_time_constant_s;

    state_.pitch += state_.pitch_rate * dt;
    state_.roll += state_.roll_rate * dt;
}

/*
Translation: vertical force balance (lift vs weight), attitude-induced horizontal accelerations
and the configured linear wind disturbance, then explicit Euler integration. Ground
contact locked at z = 0.
*/
void Aircraft::update_translation(std::float64_t dt)
{
    const std::float64_t current_mass = parameters_.mass_kg * (1.0 + dispersion_.mass_variation);
    const std::float64_t lift = parameters_.lift_coefficient * state_.actual_rpm * state_.actual_rpm;
    const std::float64_t weight = current_mass * parameters_.gravity_mps2;
    const std::float64_t az = (lift - weight) / current_mass;
    const std::float64_t wind_gain = state_.z > 0.0 ? parameters_.wind_gain_per_s : 0.0;
    const std::float64_t ax = parameters_.pitch_acceleration_gain_mps2_per_rad * state_.pitch + wind_gain * wind_x_mps_;
    const std::float64_t ay = parameters_.roll_acceleration_gain_mps2_per_rad * state_.roll + wind_gain * wind_y_mps_;

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
