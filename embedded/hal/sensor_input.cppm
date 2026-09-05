/*
Filename: embedded/hal/sensor_input.cppm
Description: HAL abstraction over the sensor acquisition path. The PC mock
(SimulatedSensorInput) and the STM32 HIL implementation both satisfy this
interface, keeping the control core transport-agnostic.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.hal.sensor;

import std;

import flight.hal.types;

export namespace FlightCore::HAL
{

/*
    Reads a fresh sensor sample into out_data. Returns false when no new sample
    is available, so the caller can apply its time/deadline handling instead of
    blocking (HIL-Proto v1.0 PC-master lockstep).
*/
class ISensorInput
{
public:
    ISensorInput() = default;
    virtual ~ISensorInput() = default;

    ISensorInput(const ISensorInput&)            = delete;
    ISensorInput& operator=(const ISensorInput&) = delete;

    [[nodiscard]] virtual bool readSensorData(SensorData& out_data) noexcept = 0;
};

}
