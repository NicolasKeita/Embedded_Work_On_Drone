/*
Filename: Src/Simulation/Aircraft.cpp
Description: Aircraft update pipeline wiring actuators to physics to sensors.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Aircraft;

import std;

import Actuators;
import PhysicsModel;
import Sensors;

void Aircraft::SetCommand(const ControlCommand& command)
{
    m_actuators.Apply(command);
}

void Aircraft::Update(double dt)
{
    const double thrust = m_actuators.GetThrustNewtons();

    m_physics.Update(dt, thrust);
    m_state = m_physics.GetState();
    m_sensors.Read(m_state);
}

const AircraftState& Aircraft::GetState() const
{
    return m_state;
}

const Sensors& Aircraft::GetSensors() const
{
    return m_sensors;
}
