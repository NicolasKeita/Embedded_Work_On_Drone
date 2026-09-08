/*
Filename: Src/Simulation/Aircraft-Core.cpp
Description: Integration of the simplified Heliblade-like flight physics (actuators, attitude, translation).

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

    constexpr std::float64_t kMinRpm = 0.0;
    constexpr std::float64_t kMaxRpm = 12000.0;
    constexpr std::float64_t kMinServoDeg = -30.0;
    constexpr std::float64_t kMaxServoDeg = 30.0;

    std::float64_t Clamp(std::float64_t value, std::float64_t minValue, std::float64_t maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

Aircraft::Aircraft(const sim::PhysicsDispersion& dispersion)
    : dispersion_{dispersion}
{
}

void Aircraft::set_dispersion(const sim::PhysicsDispersion& dispersion)
{
    dispersion_ = dispersion;
}

const sim::PhysicsDispersion& Aircraft::dispersion() const
{
    return dispersion_;
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
    const std::float64_t current_mass = kBaseMassKg * (1.0 + dispersion_.mass_variation);

    return std::sqrt(current_mass * kGravityMps2 / kLiftCoeff);
}
