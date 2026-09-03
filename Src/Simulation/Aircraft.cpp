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
    constexpr std::float64_t kGravityMps2 = 9.81;
    constexpr std::float64_t kMassKg = 1.2;

    constexpr std::float64_t kLiftCoeff = 1.77e-5;
    constexpr std::float64_t kPitchTargetGainRadPerDeg = 0.01;
    constexpr std::float64_t kRollTargetGainRadPerDeg = 0.01;
    constexpr std::float64_t kPitchAccelGainMps2PerRad = 9.81;
    constexpr std::float64_t kRollAccelGainMps2PerRad = 9.81;

    constexpr std::float64_t kAttitudeTauS = 0.25;

    constexpr std::float64_t kMinRpm = 0.0;
    constexpr std::float64_t kMaxRpm = 12000.0;
    constexpr std::float64_t kMinServoDeg = -30.0;
    constexpr std::float64_t kMaxServoDeg = 30.0;

    std::float64_t Clamp(std::float64_t value, std::float64_t minValue, std::float64_t maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

/*
Perfect actuators (effective state = command).
Extension point: add a first-order dynamics here (motor / servo inertia)
between command_ and state_.
*/
void Aircraft::update_actuators()
{
    state_.actual_rpm = Clamp(command_.wing_rpm, kMinRpm, kMaxRpm);
    state_.actual_left_servo = Clamp(command_.left_servo_angle, kMinServoDeg, kMaxServoDeg);
    state_.actual_right_servo = Clamp(command_.right_servo_angle, kMinServoDeg, kMaxServoDeg);
}

/*
Smooth first-order dynamics: no instantaneous attitude jump, the servo mean
(pitch) and differential (roll) commands are tracked with the time constant
kAttitudeTauS.
*/
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
Translation: vertical force balance (lift vs weight), attitude-induced
horizontal accelerations, then explicit Euler integration:
force -> acceleration -> velocity -> position.
Ground contact: locked at z = 0 while the vertical velocity is downward.
*/
void Aircraft::update_translation(std::float64_t dt)
{
    const std::float64_t lift = kLiftCoeff * state_.actual_rpm * state_.actual_rpm;
    const std::float64_t weight = kMassKg * kGravityMps2;
    const std::float64_t az = (lift - weight) / kMassKg;

    const std::float64_t ax = kPitchAccelGainMps2PerRad * state_.pitch;
    const std::float64_t ay = kRollAccelGainMps2PerRad * state_.roll;

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

void Aircraft::update(std::float64_t dt)
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

std::float64_t Aircraft::hover_rpm() const
{
    return std::sqrt(kMassKg * kGravityMps2 / kLiftCoeff);
}
