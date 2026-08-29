/*
Filename: Src/SIL/ActuatorFaultInjector.cpp
Description: Actuator fault injection implementation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ActuatorFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

namespace sim::sil {

void ActuatorFaultInjector::inject(SimulationState& state, double current_time)
{
    if (is_active(current_time)) {
        state.actuator_efficiency = scenario_.parameters.efficiency;
    }
}

}
