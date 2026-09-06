/*
Filename: Src/Embedded/Sim/SimSensorInput.cpp
Description: Implementation of the PC sensor-acquisition mock (SimSensorInput)
handing simulated samples to the Flight Controller through ISensorInput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SimSensorInput;

import std;

import HalTypes;
import SensorInput;

namespace FlightCore::Sim
{

void SimulatedSensorInput::inject(const FlightCore::HAL::SensorData& data) noexcept
{
    latest_ = data;
    fresh_ = true;
}

bool SimulatedSensorInput::readSensorData(FlightCore::HAL::SensorData& out_data) noexcept
{
    if (!fresh_) {
        return false;
    }
    out_data = latest_;
    fresh_ = false;
    return true;
}

}
