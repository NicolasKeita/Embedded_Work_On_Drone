/*
Filename: Src/SIL/Core/Telemetry/Telemetry-Chain.cppm
Description: Simulated sensor chain, range validation layer and environment-side corruption.

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

/*
Measurement set produced by the simulated sensor chain (GNSS/IMU/actuator
feedback). This is the only data path the flight controller is allowed to
consume: the physics ground truth stays in AircraftState and never reaches the
FC directly.
*/
struct SensorTelemetry {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    double pitch = 0.0;
    double roll = 0.0;
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
[[nodiscard]] AircraftState to_aircraft_state(const SensorTelemetry& telemetry);
[[nodiscard]] SensorValidity validate(const SensorTelemetry& telemetry, const SensorValidationLimits& limits);
SensorTelemetry apply_corruption(const SensorTelemetry& telemetry,
                                 SensorCorruptionMode mode, double corrupted_altitude_m);
[[nodiscard]] std::string_view corruption_mode_name(SensorCorruptionMode mode);

}
