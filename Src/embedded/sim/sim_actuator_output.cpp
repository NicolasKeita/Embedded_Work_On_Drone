/*
Filename: Src/embedded/sim/sim_actuator_output.cpp
Description: Implementation of the PC actuator-command mock (flight.sim.actuator)
recording the last command set emitted by the Flight Controller.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.sim.actuator;

import flight.hal.actuator;
import flight.hal.types;
import std;

namespace FlightCore::Sim
{

bool SimulatedActuatorOutput::writeActuatorCommands(const FlightCore::HAL::ActuatorCommands& in_cmds) noexcept
{
    last_ = in_cmds;
    pending_ = true;
    return true;
}

}
