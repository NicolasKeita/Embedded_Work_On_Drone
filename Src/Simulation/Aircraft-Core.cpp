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
    sim::AircraftParameters current_parameters{};

    /* Bounds an actuator command using the model limits. */
    std::float64_t Clamp(std::float64_t value, std::float64_t minValue, std::float64_t maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

/* Returns a value snapshot so live aircraft do not depend on mutable defaults. */
sim::AircraftParameters sim::aircraft_parameters() noexcept
{
    return current_parameters;
}

/* Sets the model defaults during startup, before any concurrent simulation work. */
void sim::set_aircraft_parameters(const sim::AircraftParameters& parameters) noexcept
{
    current_parameters = parameters;
}

/* Captures the startup model defaults and the supplied Monte Carlo dispersion. */
Aircraft::Aircraft(const sim::PhysicsDispersion& dispersion)
    : dispersion_{dispersion}
{
}

/* Updates the run dispersion while retaining this aircraft's model parameters. */
void Aircraft::set_dispersion(const sim::PhysicsDispersion& dispersion)
{
    dispersion_ = dispersion;
}

/* Returns the dispersion currently applied to the model. */
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
    state_.actual_rpm = Clamp(command_.wing_rpm, parameters_.minimum_rpm, parameters_.maximum_rpm);
    state_.actual_left_servo = Clamp(command_.left_servo_angle, parameters_.minimum_servo_deg, parameters_.maximum_servo_deg);
    state_.actual_right_servo = Clamp(command_.right_servo_angle, parameters_.minimum_servo_deg, parameters_.maximum_servo_deg);
}

/* Advances the model by one simulation interval. */
void Aircraft::update(std::float64_t dt)
{
    update_actuators();
    update_attitude(dt);
    update_translation(dt);
}

/* Stores the actuator commands for the next integration step. */
void Aircraft::set_command(const ControlCommand& cmd)
{
    command_ = cmd;
}

/* Returns the integrated aircraft state. */
const AircraftState& Aircraft::state() const
{
    return state_;
}

/* Computes the speed needed to balance the configured weight and lift. */
std::float64_t Aircraft::hover_rpm() const
{
    const std::float64_t nominal_rpm = std::sqrt(parameters_.mass_kg * parameters_.gravity_mps2 / parameters_.lift_coefficient);
    return nominal_rpm * std::sqrt(1.0 + dispersion_.mass_variation);
}
