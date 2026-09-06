/*
Filename: Src/Embedded/Sim/SimSensorInput.cppm
Description: PC mock of the sensor acquisition path. The aircraft simulator injects
a simulated sensor sample (a measurement, never the ground-truth state, per
hil_validation.md 4.3) which the Flight Controller reads through ISensorInput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SimSensorInput;

import std;

import HalTypes;
import SensorInput;

export namespace FlightCore::Sim
{

class SimulatedSensorInput final : public FlightCore::HAL::ISensorInput
{
public:
    SimulatedSensorInput() = default;

    /* Called by the PC simulator once a simulated sensor sample is produced. */
    void inject(const FlightCore::HAL::SensorData& data) noexcept;

    [[nodiscard]] bool hasFreshData() const noexcept;

    [[nodiscard]] bool readSensorData(FlightCore::HAL::SensorData& out_data) noexcept override;

private:
    FlightCore::HAL::SensorData latest_{};
    bool                        fresh_{false};
};

inline bool SimulatedSensorInput::hasFreshData() const noexcept
{
    return fresh_;
}

}
