/*
Filename: Src/SIL/Faults/FaultInjectors-Injectors.cpp
Description: Concrete fault injection implementations (FC1, communication, sensor, actuator).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;

namespace sim::sil {

void FCFailureInjector::inject(SimulationState& state, double current_time)
{
    if (is_active(current_time)) {
        state.fc1_alive = false;
    }
}

void CommunicationFaultInjector::inject(SimulationState& state, double current_time)
{
    if (!is_active(current_time)) {
        return;
    }
    if (scenario_.fault_type == FaultType::CommunicationLoss) {
        state.comms_link_up = false;
    }
    else {
        state.comms_loss_probability = scenario_.parameters.loss_probability;
    }
}

void SensorFaultInjector::inject(SimulationState& state, double current_time)
{
    if (!is_active(current_time)) {
        return;
    }
    state.sensor_corruption = scenario_.parameters.corruption;
    state.corrupted_altitude_m = scenario_.parameters.corrupted_altitude_m;
}

void ActuatorFaultInjector::inject(SimulationState& state, double current_time)
{
    if (is_active(current_time)) {
        state.actuator_efficiency = scenario_.parameters.efficiency;
    }
}

}
