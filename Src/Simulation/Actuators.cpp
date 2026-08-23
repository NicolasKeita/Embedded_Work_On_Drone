/*
Filename: Src/Simulation/Actuators.cpp
Description: Command clamping and thrust computation for the engine actuators.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Actuators;

import std;

namespace
{
    constexpr double kMinRpm = 0.0;
    constexpr double kMaxRpm = 12000.0;
    constexpr double kMinServoDeg = -30.0;
    constexpr double kMaxServoDeg = 30.0;
    // Meme coefficient que le PhysicsModel attend : la poussee vient des moteurs.
    constexpr double kThrustCoeff = 1.77e-5;
}

namespace
{
    double Clamp(double value, double minValue, double maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

void Actuators::Apply(const ControlCommand& command)
{
    m_command.rpm = Clamp(command.rpm, kMinRpm, kMaxRpm);
    m_command.servoLeftDeg = Clamp(command.servoLeftDeg, kMinServoDeg, kMaxServoDeg);
    m_command.servoRightDeg = Clamp(command.servoRightDeg, kMinServoDeg, kMaxServoDeg);
}

double Actuators::GetThrustNewtons() const
{
    return kThrustCoeff * m_command.rpm * m_command.rpm;
}

const ControlCommand& Actuators::GetCommand() const
{
    return m_command;
}
