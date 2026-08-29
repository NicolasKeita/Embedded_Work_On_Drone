/*
Filename: Src/SIL/Telemetry.cppm
Description: Sensor telemetry, range validation layer and environment-side corruption.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Telemetry;

import std;

import Aircraft;

export namespace sim::sil {

enum class SensorCorruptionMode { None, AltitudeNaN, AltitudeOutOfRange, ExtremeNoise };

struct SensorValidationLimits {
    double max_altitude_m = 500.0;
    double max_position_m = 1000.0;
};

struct SensorTelemetry {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double actual_rpm = 0.0;
    double actual_left_servo = 0.0;
    double actual_right_servo = 0.0;
};

struct SensorValidity {
    bool altitude_valid = true;
    bool position_valid = true;

    [[nodiscard]] bool all_valid() const noexcept;
};

[[nodiscard]] SensorTelemetry make_telemetry(const AircraftState& state);

[[nodiscard]] SensorValidity validate(const SensorTelemetry& telemetry,
                                      const SensorValidationLimits& limits);

SensorTelemetry apply_corruption(const SensorTelemetry& telemetry,
                                 SensorCorruptionMode mode, double corrupted_altitude_m);

}
