/*
Filename: Src/SIL/Core/Telemetry/SilTelemetry.cpp
Description: Fixed-rate dual telemetry sampling implementations (sensor path and ground truth).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilTelemetry;

import std;

import Aircraft;
import Telemetry;

namespace sim::sil {

namespace {

/*
Builds the sensor-path sample of one recording instant.
*/
TelemetrySample make_sensor_sample(std::float64_t              time,
                                   const TelemetrySampleInput& sample_input)
{
    return TelemetrySample{.time = time,
                           .x = sample_input.sensor.x,
                           .y = sample_input.sensor.y,
                           .altitude_m = sample_input.sensor.z,
                           .pitch_rad = sample_input.sensor.pitch,
                           .roll_rad = sample_input.sensor.roll,
                           .vx = sample_input.sensor.vx,
                           .vy = sample_input.sensor.vy,
                           .vz = sample_input.sensor.vz,
                           .commanded_rpm = sample_input.commanded_rpm,
                           .actual_rpm = sample_input.sensor.actual_rpm,
                           .left_servo_deg = sample_input.command.left_servo_angle,
                           .right_servo_deg = sample_input.command.right_servo_angle,
                           .target_x = sample_input.control.target_x,
                           .target_y = sample_input.control.target_y,
                           .target_z = sample_input.control.target_z,
                           .mission_state = sample_input.control.mission_state,
                           .safety_state = sample_input.control.safety_state};
}

/*
Builds the aligned ground-truth sample of one recording instant.
*/
TrueStateSample make_truth_sample(std::float64_t time, const AircraftState& truth)
{
    return TrueStateSample{.time = time,
                           .x = truth.x,
                           .y = truth.y,
                           .z = truth.z,
                           .vx = truth.vx,
                           .vy = truth.vy,
                           .vz = truth.vz,
                           .pitch = truth.pitch,
                           .roll = truth.roll,
                           .actual_rpm = truth.actual_rpm,
                           .actual_left_servo = truth.actual_left_servo,
                           .actual_right_servo = truth.actual_right_servo};
}

}

/*
Samples the sensor chain (flight-controller view) and the physics ground truth
at the configured interval: simulation-time based, deterministic and
independent from the internal integration tick. Both streams share the same
timestamps so that post-run correlation stays trivial.
*/
void TelemetryRecorder::maybe_record(std::float64_t              time,
                                     const AircraftState&        truth,
                                     const TelemetrySampleInput& sample_input)
{
    if (interval_s <= 0.0) {
        return;
    }
    if (time - last_sample_time + 1.0e-9 < interval_s) {
        return;
    }
    last_sample_time = time;

    samples.push_back(make_sensor_sample(time, sample_input));
    truth_samples.push_back(make_truth_sample(time, truth));
}

}
