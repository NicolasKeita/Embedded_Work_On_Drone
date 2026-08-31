/*
Filename: Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
Description: Range validation, environment-side corruption and corruption naming.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Telemetry;

import std;

import Aircraft;

namespace sim::sil {

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

/*
Human-readable name of a sensor corruption mode for fault event reports.
*/
std::string_view corruption_mode_name(SensorCorruptionMode mode)
{
    switch (mode) {
    case SensorCorruptionMode::None:
        return "NONE";
    case SensorCorruptionMode::AltitudeNaN:
        return "ALTITUDE_NAN";
    case SensorCorruptionMode::AltitudeOutOfRange:
        return "ALTITUDE_OUT_OF_RANGE";
    case SensorCorruptionMode::ExtremeNoise:
        return "EXTREME_NOISE";
    }
    return "UNKNOWN";
}

}