/*
Filename: Src/Embedded/Hil/HilSensorModel.cppm
Description: Simulated sensor chain for the HIL runner. Produces a SensorTelemetry
measurement from the private aircraft ground truth (never the ground truth itself),
optionally adding deterministic seeded noise so truth and sensor streams stay
distinguishable. Helpers expand the chain into the HIL-Proto wire SensorData and
collapse a received SensorData back into the sensor chain / the Flight Controller
input view, so the controller only ever consumes a measurement. SIL's validation
and corruption helpers are reused so the detection chain matches the SIL baseline.
Exports:
    class HilSensorModel,
    to_sensor_data(),
    to_telemetry(),
    to_aircraft_state()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilSensorModel;

import std;

import Aircraft;
import HalTypes;
import Telemetry;

export namespace sim::hil {

/*
Deterministic, seeded measurement model. sample() returns a SensorTelemetry with
optional Gaussian noise per channel; with zero standard deviation the sensors are
ideal and the run is bit-reproducible (matching the validated SIL nominal case).
*/
class HilSensorModel {
public:
    HilSensorModel() = default;
    HilSensorModel(std::float64_t noise_stddev, std::uint64_t seed);

    [[nodiscard]] sim::sil::SensorTelemetry sample(const AircraftState& truth);

private:
    std::mt19937_64                    generator_{42};
    std::normal_distribution<std::float64_t> distribution_{0.0, 0.0};
};

/*
Builds the HIL-Proto wire SensorData from the (possibly corrupted) sensor chain,
carrying the unused gyroscope from the ground truth, leaving accelerometers at
rest and setting the validity bitmask from the validated sensor chain.
*/
[[nodiscard]] FlightCore::HAL::SensorData to_sensor_data(const sim::sil::SensorTelemetry& sensors,
                                                          std::uint64_t sim_timestamp_us,
                                                          const AircraftState& truth,
                                                          const sim::sil::SensorValidity& validity) noexcept;

/* Collapses a received wire SensorData back into the sensor chain summary. */
[[nodiscard]] sim::sil::SensorTelemetry to_telemetry(const FlightCore::HAL::SensorData& sensor) noexcept;

/* Rebuilds the Flight Controller input view from a received SensorData. */
[[nodiscard]] AircraftState to_aircraft_state(const FlightCore::HAL::SensorData& sensor) noexcept;

}
