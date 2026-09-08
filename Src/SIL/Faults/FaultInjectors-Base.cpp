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
meaning active until the end of the simulation. A nominal scenario (NONE) is
never active.
*/
bool FaultInjector::is_active(std::float64_t current_time) const
{
    if (scenario_.failure_mode == FailureMode::NONE) {
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
Dispatches the owned scenario to the failure-mode-specific injection helper.
*/
void FaultInjector::inject(SimulationState& state, std::float64_t current_time) const
{
    if (!is_active(current_time)) {
        return;
    }
    switch (scenario_.failure_mode) {
    case FailureMode::FC1_UNAVAILABLE:
        inject_fc1_unavailable(scenario_, state);
        break;
    case FailureMode::FC_COMMUNICATION_LOSS:
        inject_communication_loss(scenario_, state);
        break;
    case FailureMode::COMMUNICATION_DEGRADED:
        inject_communication_degraded(scenario_, state);
        break;
    case FailureMode::INVALID_SENSOR_DATA:
        inject_invalid_sensor_data(scenario_, state);
        break;
    case FailureMode::ACTUATOR_DEGRADED:
        inject_actuator_degraded(scenario_, state);
        break;
    case FailureMode::NONE:
    case FailureMode::CONTROL_DEADLINE_MISSED:
    case FailureMode::INVALID_NUMERICAL_STATE:
        break;
    }
}

}
