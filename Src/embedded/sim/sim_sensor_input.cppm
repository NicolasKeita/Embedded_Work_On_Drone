/*
Filename: Src/embedded/sim/sim_sensor_input.cppm
Description: PC mock of the sensor acquisition path. The aircraft simulator injects
a simulated sensor sample (a measurement, never the ground-truth state, per
hil_validation.md 4.3) which the Flight Controller reads through ISensorInput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.sim.sensor;

import std;

import flight.hal.types;
import flight.hal.sensor;

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
    bool fresh_{false};
};

/* Records the latest simulated sample and marks it as pending consumption. */
inline void SimulatedSensorInput::inject(const FlightCore::HAL::SensorData& data) noexcept
{
    latest_ = data;
    fresh_ = true;
}

inline bool SimulatedSensorInput::hasFreshData() const noexcept
{
    return fresh_;
}

/* Returns the pending sample once, then clears the pending flag (lockstep). */
inline bool SimulatedSensorInput::readSensorData(FlightCore::HAL::SensorData& out_data) noexcept
{
    if (!fresh_) return false;
    out_data = latest_;
    fresh_ = false;
    return true;
}

}
