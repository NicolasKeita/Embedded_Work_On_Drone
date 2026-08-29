/*
Filename: Src/SIL/Faults/CommunicationFaultInjector.cpp
Description: Communication fault injection implementation (cutoff or loss rate).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module CommunicationFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

namespace sim::sil {

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

}
