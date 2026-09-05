/*
Filename: embedded/hal/actuator_output.cppm
Description: HAL abstraction over the actuator command path. The PC mock
(SimulatedActuatorOutput) records the last command set for the simulator; the
STM32 HIL implementation encodes and emits an ActuatorPacket over the transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.hal.actuator;

import std;

import flight.hal.types;

export namespace FlightCore::HAL
{

/*
    Writes the actuator command set. Returns false if the downstream channel
    cannot accept the command (transport full / hardware fault), letting the
    caller escalate through its safety path.
*/
class IActuatorOutput
{
public:
    IActuatorOutput() = default;
    virtual ~IActuatorOutput() = default;

    IActuatorOutput(const IActuatorOutput&)            = delete;
    IActuatorOutput& operator=(const IActuatorOutput&) = delete;

    [[nodiscard]] virtual bool writeActuatorCommands(const ActuatorCommands& in_cmds) noexcept = 0;
};

}
