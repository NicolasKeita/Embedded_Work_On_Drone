/*
Filename: Src/Simulation/PhysicsModel.cppm
Description: Simple vertical flight physics producing the aircraft state from thrust.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module PhysicsModel;

import std;

export struct AircraftState
{
    double altitudeMeters = 0.0;
    double verticalSpeedMps = 0.0;
};

export class PhysicsModel
{
public:
    void Update(double dt, double thrustNewtons);
    const AircraftState& GetState() const;

private:
    AircraftState m_state;
};
