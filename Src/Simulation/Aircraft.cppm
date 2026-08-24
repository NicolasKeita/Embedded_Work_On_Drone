/*
Filename: Src/Simulation/Aircraft.cppm
Description: Public interface of the simplified Heliblade-like physics simulation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Aircraft;

import std;

export struct ControlCommand
{
    double wing_rpm = 0.0;
    double left_servo_angle = 0.0;
    double right_servo_angle = 0.0;
};

export struct AircraftState
{
    // Position (metres).
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;

    // Vitesse lineaire (m/s).
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;

    // Attitude (radians).
    double pitch = 0.0;
    double roll = 0.0;

    // Vitesse angulaire (rad/s).
    double pitch_rate = 0.0;
    double roll_rate = 0.0;

    // Etat effectif des actionneurs.
    double actual_rpm = 0.0;
    double actual_left_servo = 0.0;
    double actual_right_servo = 0.0;
};

export class Aircraft
{
public:
    void update(double dt);
    void set_command(const ControlCommand& cmd);

    [[nodiscard]] const AircraftState& state() const;
    [[nodiscard]] double hover_rpm() const;

private:
    void update_actuators();
    void update_attitude(double dt);
    void update_translation(double dt);

    AircraftState state_{};
    ControlCommand command_{};
};
