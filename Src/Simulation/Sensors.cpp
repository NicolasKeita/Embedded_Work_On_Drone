/*
Filename: Src/Simulation/Sensors.cpp
Description: Perfect sensor readings of the current aircraft state.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Sensors;

import std;

import PhysicsModel;

void Sensors::Read(const AircraftState& state)
{
    m_reading = state;
}

double Sensors::GetAltitudeMeters() const
{
    return m_reading.altitudeMeters;
}

double Sensors::GetVerticalSpeedMps() const
{
    return m_reading.verticalSpeedMps;
}
