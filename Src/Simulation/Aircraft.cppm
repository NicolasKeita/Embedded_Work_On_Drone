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
    void update(std::float64_t dt);
    void set_command(const ControlCommand& cmd);

    [[nodiscard]] const AircraftState& state() const;
    [[nodiscard]] std::float64_t hover_rpm() const;

private:
    void update_actuators();
    void update_attitude(std::float64_t dt);
    void update_translation(std::float64_t dt);

    AircraftState  state_{};
    ControlCommand command_{};
};
