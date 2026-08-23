/*
Filename: Src/Simulation/Aircraft.cppm
Description: Central aircraft aggregating physics model, sensors and actuators.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Aircraft;

import std;

import PhysicsModel;
import Sensors;
import Actuators;

export class Aircraft
{
public:
    void SetCommand(const ControlCommand& command);
    void Update(double dt);

    const AircraftState& GetState() const;
    const Sensors& GetSensors() const;

private:
    AircraftState m_state;
    PhysicsModel m_physics;
    Sensors m_sensors;
    Actuators m_actuators;
};
