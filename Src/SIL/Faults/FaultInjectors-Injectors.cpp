/*
Filename: Src/SIL/Faults/FaultInjectors-Injectors.cpp
Description: Domain-specific fault injection implementations (FC1, communication, sensor, actuator).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;

namespace sim::sil {

/*
Interrupts FC1 heartbeat and status emission (crash / silence).
*/
void inject_fc1_failure(const FaultScenario&, SimulationState& state)
{
    state.fc1_alive = false;
}

/*
Applies a total communication cutoff or a random packet loss rate.
*/
void inject_communication_fault(const FaultScenario& scenario, SimulationState& state)
{
    if (scenario.fault_type == FaultType::CommunicationLoss) {
        state.comms_link_up = false;
    }
    else {
        state.comms_loss_probability = scenario.parameters.loss_probability;
    }
}

/*
Corrupts the sensor telemetry seen by bus consumers (out-of-range, NaN or
extreme noise).
*/
void inject_sensor_fault(const FaultScenario& scenario, SimulationState& state)
{
    state.sensor_corruption = scenario.parameters.corruption;
    state.corrupted_altitude_m = scenario.parameters.corrupted_altitude_m;
}

/*
Alters the actuator dynamics (efficiency loss, thrust reduction).
*/
void inject_actuator_fault(const FaultScenario& scenario, SimulationState& state)
{
    state.actuator_efficiency = scenario.parameters.efficiency;
}

}
