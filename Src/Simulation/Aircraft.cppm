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
    Aircraft() = default;
    explicit Aircraft(const sim::PhysicsDispersion& dispersion);

    void update(std::float64_t dt);
    /* Sets the horizontal ambient wind velocity in world coordinates (m/s). */
    void set_wind(std::float64_t x_mps, std::float64_t y_mps) noexcept;
    void set_command(const ControlCommand& cmd);
    void set_dispersion(const sim::PhysicsDispersion& dispersion);

    [[nodiscard]] const AircraftState& state() const;
    [[nodiscard]] std::float64_t hover_rpm() const;
    [[nodiscard]] const sim::PhysicsDispersion& dispersion() const;

private:
    void update_actuators();
    void update_attitude(std::float64_t dt);
    void update_translation(std::float64_t dt);

    std::float64_t wind_x_mps_ = 0.0;
    std::float64_t wind_y_mps_ = 0.0;
    AircraftState          state_{};
    ControlCommand         command_{};
    sim::PhysicsDispersion dispersion_{};
};
