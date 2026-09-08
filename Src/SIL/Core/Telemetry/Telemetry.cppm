/*
Filename: Src/SIL/Core/Telemetry/Telemetry.cppm
Description: Simulated sensor chain, range validation layer and environment-side corruption.
Exports:
    enum class SensorCorruptionMode,
    struct SensorValidationLimits,
    struct SensorTelemetry,
    struct SensorValidity,
    make_telemetry(),
    to_aircraft_state(),
    validate(),
    apply_corruption(),
    corruption_mode_name()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Telemetry;

import std;

import Aircraft;

export namespace sim::sil {

enum class SensorCorruptionMode { None, AltitudeNaN, AltitudeOutOfRange, ExtremeNoise };

// Peak amplitude of the deterministic extreme-noise disturbance (meters).
inline constexpr std::float64_t kExtremeNoiseAmplitudeM = 300.0;

struct SensorValidationLimits {
    std::float64_t max_altitude_m = 500.0;
    std::float64_t max_position_m = 1000.0;
};

/*
Measurement set produced by the simulated sensor chain (GNSS/IMU/actuator
feedback). This is the only data path the flight controller is allowed to
consume: the physics ground truth stays in AircraftState and never reaches the
FC directly.
*/
struct SensorTelemetry {
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;
    std::float64_t vx = 0.0;
    std::float64_t vy = 0.0;
    std::float64_t vz = 0.0;
    std::float64_t pitch = 0.0;
    std::float64_t roll = 0.0;
    std::float64_t actual_rpm = 0.0;
    std::float64_t actual_left_servo = 0.0;
    std::float64_t actual_right_servo = 0.0;
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
                                 SensorCorruptionMode mode, std::float64_t corrupted_altitude_m);
[[nodiscard]] std::string_view corruption_mode_name(SensorCorruptionMode mode);

}
