/*
Filename: Src/SIL/FCFailureInjector.cpp
Description: FC1 failure injection implementation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FCFailureInjector;

import std;

import IFaultInjector;
import SilTypes;

namespace sim::sil {

void FCFailureInjector::inject(SimulationState& state, double current_time)
{
    if (is_active(current_time)) {
        state.fc1_alive = false;
    }
}

}
