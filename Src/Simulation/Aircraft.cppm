/*
Filename: Src/Simulation/Aircraft.cppm
Description: Public interface of the simplified Heliblade-like physics simulation.
Exports:
    struct ControlCommand,
    struct AircraftState,
    class Aircraft

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Aircraft;

import std;

import PhysicsDispersion;

export inline constexpr std::float64_t kNominalAircraftHoverRpm = 815.527280820643;

export namespace sim
{
    /* Numeric model parameters copied into each newly constructed aircraft. */
    struct AircraftParameters
    {
        std::float64_t mass_kg = 1.2;
        std::float64_t gravity_mps2 = 9.81;
        std::float64_t lift_coefficient = 1.77e-5;
        std::float64_t pitch_target_gain_rad_per_deg = 0.01;
        std::float64_t roll_target_gain_rad_per_deg = 0.01;
        std::float64_t pitch_acceleration_gain_mps2_per_rad = 9.81;
        std::float64_t roll_acceleration_gain_mps2_per_rad = 9.81;
        std::float64_t attitude_time_constant_s = 0.25;
        std::float64_t wind_gain_per_s = 0.08;
        std::float64_t minimum_rpm = 0.0;
        std::float64_t maximum_rpm = 12000.0;
        std::float64_t minimum_servo_deg = -30.0;
        std::float64_t maximum_servo_deg = 30.0;
    };

    /* Returns the model parameters used by subsequently constructed aircraft. */
    [[nodiscard]] AircraftParameters aircraft_parameters() noexcept;

    /* Installs validated defaults before constructing aircraft or starting worker threads. */
    void set_aircraft_parameters(const AircraftParameters& parameters) noexcept;
}

export struct ControlCommand
{
    std::float64_t wing_rpm = 0.0;
    std::float64_t left_servo_angle = 0.0;
    std::float64_t right_servo_angle = 0.0;
};

export struct AircraftState
{
    // Position (meters).
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;

    // Linear velocity (m/s).
    std::float64_t vx = 0.0;
    std::float64_t vy = 0.0;
    std::float64_t vz = 0.0;

    // Attitude (radians).
    std::float64_t pitch = 0.0;
    std::float64_t roll = 0.0;

    // Angular velocity (rad/s).
    std::float64_t pitch_rate = 0.0;
    std::float64_t roll_rate = 0.0;

    // Effective actuator state.
    std::float64_t actual_rpm = 0.0;
    std::float64_t actual_left_servo = 0.0;
    std::float64_t actual_right_servo = 0.0;
};

export class Aircraft
{
public:
    /* Captures the installed model parameters with a nominal dispersion. */
    Aircraft() = default;
    /* Captures the installed model parameters and the supplied run dispersion. */
    explicit Aircraft(const sim::PhysicsDispersion& dispersion);

    /* Advances the actuator, attitude, and translation state by dt seconds. */
    void update(std::float64_t dt);
    /* Sets the horizontal ambient wind velocity in world coordinates (m/s). */
    void set_wind(std::float64_t x_mps, std::float64_t y_mps) noexcept;
    /* Replaces the actuator commands used by the next integration step. */
    void set_command(const ControlCommand& cmd);
    /* Replaces the physical dispersion without changing the captured model parameters. */
    void set_dispersion(const sim::PhysicsDispersion& dispersion);

    /* Returns the current integrated aircraft state. */
    [[nodiscard]] const AircraftState& state() const;
    /* Derives the hover speed from the captured mass, gravity, and lift coefficient. */
    [[nodiscard]] std::float64_t hover_rpm() const;
    /* Returns the current physical dispersion. */
    [[nodiscard]] const sim::PhysicsDispersion& dispersion() const;

private:
    /* Applies the captured speed and servo limits to the actuator commands. */
    void update_actuators();
    /* Integrates the configured first-order attitude model. */
    void update_attitude(std::float64_t dt);
    /* Integrates the configured lift, horizontal acceleration, and wind model. */
    void update_translation(std::float64_t dt);

    sim::AircraftParameters parameters_ = sim::aircraft_parameters();
    std::float64_t         wind_x_mps_ = 0.0;
    std::float64_t         wind_y_mps_ = 0.0;
    AircraftState          state_{};
    ControlCommand         command_{};
    sim::PhysicsDispersion dispersion_{};
};
