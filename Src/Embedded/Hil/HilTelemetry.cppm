/*
Filename: Src/Embedded/Hil/HilTelemetry.cppm
Description: Structured HIL telemetry streams and fixed-rate dual sampler. The
sensor stream records what the Flight Controller observed (HAL SensorData, never
the physics ground truth); the truth stream records the aircraft physics state for
validation. The two streams stay separate so the report can show
"TRUE altitude = 100.0 m / sensor altitude = 99.7 m" without ever substituting one
for the other. The internal structured rate is configurable (default 20 Hz); the
human-readable report downsamples to 1 Hz separately.
Exports:
    struct HilTruthSample,
    struct HilSensorSample,
    struct HilTelemetryControl,
    struct HilTelemetryRecorder

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilTelemetry;

import std;

import Aircraft;
import HalTypes;

export namespace sim::hil {

struct HilTruthSample {
    std::float64_t time_s = 0.0;
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;
    std::float64_t vx = 0.0;
    std::float64_t vy = 0.0;
    std::float64_t vz = 0.0;
    std::float64_t pitch_rad = 0.0;
    std::float64_t roll_rad = 0.0;
    std::float64_t actual_rpm = 0.0;
    std::float64_t actual_left_servo_deg = 0.0;
    std::float64_t actual_right_servo_deg = 0.0;
};

struct HilSensorSample {
    std::float64_t time_s = 0.0;
    std::float64_t x = 0.0;
    std::float64_t y = 0.0;
    std::float64_t z = 0.0;
    std::float64_t vx = 0.0;
    std::float64_t vy = 0.0;
    std::float64_t vz = 0.0;
    std::float64_t pitch_rad = 0.0;
    std::float64_t roll_rad = 0.0;
    std::float64_t commanded_rpm = 0.0;
    std::float64_t measured_rpm = 0.0;
    std::float64_t left_servo_deg = 0.0;
    std::float64_t right_servo_deg = 0.0;
    std::float64_t target_x = 0.0;
    std::float64_t target_y = 0.0;
    std::float64_t target_z = 0.0;
    std::uint8_t   mission_state = 0;
    std::uint8_t   safety_state = 0;
};

struct HilTelemetryControl {
    std::float64_t target_x = 0.0;
    std::float64_t target_y = 0.0;
    std::float64_t target_z = 0.0;
    std::uint8_t   mission_state = 0;
    std::uint8_t   safety_state = 0;
};

struct HilTelemetryRecorder {
    std::float64_t                interval_s = 0.05;
    std::float64_t                last_sample_time = -1.0e12;
    std::vector<HilSensorSample>  samples;
    std::vector<HilTruthSample>   truth_samples;

    void maybe_record(std::float64_t time_s, const AircraftState& truth,
                      const FlightCore::HAL::SensorData& sensor,
                      const FlightCore::HAL::ActuatorCommands& command,
                      std::float64_t commanded_rpm, const HilTelemetryControl& control);
};

}
