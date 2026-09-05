/*
Filename: Src/embedded/sim/sim_sensor_input.cpp
Description: Implementation of the PC sensor-acquisition mock (flight.sim.sensor)
handing simulated samples to the Flight Controller through ISensorInput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.sim.sensor;

import flight.hal.sensor;
import flight.hal.types;
import std;

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
