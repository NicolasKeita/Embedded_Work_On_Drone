/*
Filename: Src/SIL/Faults/FaultInjectors-Base.cpp
Description: Timed activation window and environment dispatch of the value-semantic fault injector.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;

namespace sim::sil {

FaultInjector::FaultInjector(const FaultScenario& scenario) : scenario_{scenario} {}

/*
Timed activation window [start_time, start_time + duration), a duration <= 0
meaning active until the end of the simulation. A nominal scenario (None) is
never active.
*/
bool FaultInjector::is_active(double current_time) const
{
    if (scenario_.fault_type == FaultType::None) {
        return false;
    }
    if (current_time < scenario_.start_time) {
        return false;
    }
    return scenario_.duration <= 0.0 || current_time < scenario_.start_time + scenario_.duration;
}

const FaultScenario& FaultInjector::scenario() const noexcept
{
    return scenario_;
}

/*
Dispatches the owned scenario to the domain-specific injection helper.
*/
void FaultInjector::inject(SimulationState& state, double current_time) const
{
    if (!is_active(current_time)) {
        return;
    }
    switch (scenario_.fault_type) {
    case FaultType::FC1Failure:
        inject_fc1_failure(scenario_, state);
        break;
    case FaultType::CommunicationLoss:
    case FaultType::CommunicationLossRate:
        inject_communication_fault(scenario_, state);
        break;
    case FaultType::SensorFault:
        inject_sensor_fault(scenario_, state);
        break;
    case FaultType::ActuatorDegradation:
        inject_actuator_fault(scenario_, state);
        break;
    case FaultType::None:
        break;
    }
}

}
