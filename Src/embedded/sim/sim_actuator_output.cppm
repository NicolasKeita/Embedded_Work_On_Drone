/*
Filename: Src/embedded/sim/sim_actuator_output.cppm
Description: PC mock of the actuator command path. Records the last command set
emitted by the Flight Controller so the simulator can read it back and apply it
to the actuator dynamics / aerodynamic model.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.sim.actuator;

import std;

import flight.hal.types;
import flight.hal.actuator;

export namespace FlightCore::Sim
{

class SimulatedActuatorOutput final : public FlightCore::HAL::IActuatorOutput
{
public:
    SimulatedActuatorOutput() = default;

    [[nodiscard]] bool writeActuatorCommands(const FlightCore::HAL::ActuatorCommands& in_cmds) noexcept override;

    [[nodiscard]] bool hasNewCommands() const noexcept;

    [[nodiscard]] const FlightCore::HAL::ActuatorCommands& lastCommands() const noexcept;

    /* Clears the new-command flag after the simulator has consumed the command. */
    void consume() noexcept;

private:
    FlightCore::HAL::ActuatorCommands last_{};
    bool                              pending_{false};
};

inline bool SimulatedActuatorOutput::hasNewCommands() const noexcept
{
    return pending_;
}

inline const FlightCore::HAL::ActuatorCommands& SimulatedActuatorOutput::lastCommands() const noexcept
{
    return last_;
}

inline void SimulatedActuatorOutput::consume() noexcept
{
    pending_ = false;
}

}
