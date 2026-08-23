/*
Filename: Src/Simulation/Sensors.cppm
Description: Sensors exposing the aircraft state readings (altitude, vertical speed).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Sensors;

import std;

import PhysicsModel;

export class Sensors
{
public:
    void Read(const AircraftState& state);

    double GetAltitudeMeters() const;
    double GetVerticalSpeedMps() const;

private:
    AircraftState m_reading;
};
