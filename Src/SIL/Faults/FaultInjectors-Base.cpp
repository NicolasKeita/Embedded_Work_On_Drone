/*
Filename: Src/SIL/Faults/FaultInjectors-Base.cpp
Description: Timed activation window implementation shared by fault injectors.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;

namespace sim::sil {

TimedFaultInjector::TimedFaultInjector(const FaultScenario& scenario) : scenario_{scenario} {}

bool TimedFaultInjector::is_active(double current_time) const
{
    if (current_time < scenario_.start_time) {
        return false;
    }
    return scenario_.duration <= 0.0 || current_time < scenario_.start_time + scenario_.duration;
}

FaultType TimedFaultInjector::fault_type() const
{
    return scenario_.fault_type;
}

}
