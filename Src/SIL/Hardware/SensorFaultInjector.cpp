/*
Filename: Src/SIL/Hardware/SensorFaultInjector.cpp
Description: Sensor fault injection implementation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SensorFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

namespace sim::sil {

void SensorFaultInjector::inject(SimulationState& state, double current_time)
{
    if (!is_active(current_time)) {
        return;
    }
    state.sensor_corruption = scenario_.parameters.corruption;
    state.corrupted_altitude_m = scenario_.parameters.corrupted_altitude_m;
}

}
