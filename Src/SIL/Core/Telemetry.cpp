/*
Filename: Src/SIL/Core/Telemetry.cpp
Description: Telemetry building, range validation and corruption implementations.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Telemetry;

import std;

import Aircraft;

namespace sim::sil {

bool SensorValidity::all_valid() const noexcept
{
    return altitude_valid && position_valid;
}

SensorTelemetry make_telemetry(const AircraftState& state)
{
    SensorTelemetry telemetry;
    telemetry.x = state.x;
    telemetry.y = state.y;
    telemetry.z = state.z;
    telemetry.actual_rpm = state.actual_rpm;
    telemetry.actual_left_servo = state.actual_left_servo;
    telemetry.actual_right_servo = state.actual_right_servo;
    return telemetry;
}

/*
Range check: rejection of NaN values and physical out-of-range quantities; this
layer is what invalidates a disturbed sensor.
*/
SensorValidity validate(const SensorTelemetry& telemetry, const SensorValidationLimits& limits)
{
    SensorValidity validity;
    validity.altitude_valid = !std::isnan(telemetry.z) && telemetry.z >= 0.0 && telemetry.z <= limits.max_altitude_m;
    const double horizontal = std::hypot(telemetry.x, telemetry.y);
    validity.position_valid = !std::isnan(horizontal) && horizontal <= limits.max_position_m;
    return validity;
}

/*
Environment-side sensor corruption (injector effect as seen by bus consumers):
NaN altitude, forced out-of-range altitude or extreme deterministic noise
superimposed on the true measurement.
*/
SensorTelemetry apply_corruption(const SensorTelemetry& telemetry,
                                 SensorCorruptionMode   mode,
                                 double                 corrupted_altitude_m)
{
    SensorTelemetry corrupted = telemetry;
    switch (mode) {
    case SensorCorruptionMode::AltitudeNaN:
        corrupted.z = std::numeric_limits<double>::quiet_NaN();
        break;
    case SensorCorruptionMode::AltitudeOutOfRange:
        corrupted.z = corrupted_altitude_m;
        break;
    case SensorCorruptionMode::ExtremeNoise:
        corrupted.z += 300.0 * std::sin(97.0 * telemetry.z);
        break;
    case SensorCorruptionMode::None:
        break;
    }
    return corrupted;
}

}
