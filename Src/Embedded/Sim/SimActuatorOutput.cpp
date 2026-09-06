/*
Filename: Src/Embedded/Sim/SimActuatorOutput.cpp
Description: Implementation of the PC actuator-command mock (SimActuatorOutput)
recording the last command set emitted by the Flight Controller.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SimActuatorOutput;

import std;

import ActuatorOutput;
import HalTypes;

namespace FlightCore::Sim
{

bool SimulatedActuatorOutput::writeActuatorCommands(const FlightCore::HAL::ActuatorCommands& in_cmds) noexcept
{
    last_ = in_cmds;
    pending_ = true;
    return true;
}

}
