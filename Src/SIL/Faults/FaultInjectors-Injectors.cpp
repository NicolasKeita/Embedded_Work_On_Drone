/*
Filename: Src/SIL/Faults/FaultInjectors-Injectors.cpp
Description: Failure-mode injection implementations : physical representation of each failure mode.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;

namespace sim::sil {

/*
FC1_UNAVAILABLE: FC1 stops executing, which interrupts its heartbeat and
status emission (crash / silence).
*/
void inject_fc1_unavailable(const FaultScenario&, SimulationState& state)
{
    state.fc1_alive = false;
}

/*
FC_COMMUNICATION_LOSS: total cutoff of the FC1-FC2 communication path while
FC1 keeps running.
*/
void inject_communication_loss(const FaultScenario&, SimulationState& state)
{
    state.comms_link_up = false;
}

/*
COMMUNICATION_DEGRADED: intermittent packet loss at the configured
probability; the link stays up.
*/
void inject_communication_degraded(const FaultScenario& scenario, SimulationState& state)
{
    state.comms_loss_probability = scenario.parameters.loss_probability;
}

/*
INVALID_SENSOR_DATA: corrupts the sensor telemetry seen by bus consumers
(out-of-range, NaN or extreme noise). Only the altitude channel has an
injection path.
*/
void inject_invalid_sensor_data(const FaultScenario& scenario, SimulationState& state)
{
    state.sensor_corruption = scenario.parameters.corruption;
    state.corrupted_altitude_m = scenario.parameters.corrupted_altitude_m;
}

/*
ACTUATOR_DEGRADED: alters the rotation-system dynamics (efficiency loss,
thrust reduction). Only the main rotor has an injection path.
*/
void inject_actuator_degraded(const FaultScenario& scenario, SimulationState& state)
{
    state.actuator_efficiency = scenario.parameters.efficiency;
}

}
