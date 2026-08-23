/*
Filename: Src/Simulation/Actuators.cppm
Description: Actuators holding the control command and computing engine thrust.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Actuators;

import std;

export struct ControlCommand
{
    double rpm = 0.0;
    double servoLeftDeg = 0.0;
    double servoRightDeg = 0.0;
};

export class Actuators
{
public:
    void Apply(const ControlCommand& command);

    double GetThrustNewtons() const;
    const ControlCommand& GetCommand() const;

private:
    ControlCommand m_command;
};
