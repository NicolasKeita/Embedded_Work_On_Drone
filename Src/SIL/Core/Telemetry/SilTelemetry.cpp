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
TelemetrySample make_sensor_sample(double time, const SensorTelemetry& sensor,
                                   const ControlCommand& command, double commanded_rpm,
                                   const TelemetryControl& control)
{
    TelemetrySample sample;

    sample.time = time;
    sample.x = sensor.x;
    sample.y = sensor.y;
    sample.altitude_m = sensor.z;
    sample.pitch_rad = sensor.pitch;
    sample.roll_rad = sensor.roll;
    sample.vx = sensor.vx;
    sample.vy = sensor.vy;
    sample.vz = sensor.vz;
    sample.commanded_rpm = commanded_rpm;
    sample.actual_rpm = sensor.actual_rpm;
    sample.left_servo_deg = command.left_servo_angle;
    sample.right_servo_deg = command.right_servo_angle;
    sample.target_x = control.target_x;
    sample.target_y = control.target_y;
    sample.target_z = control.target_z;
    sample.mission_state = control.mission_state;
    sample.safety_state = control.safety_state;
    return sample;
}

/*
Builds the aligned ground-truth sample of one recording instant.
*/
TrueStateSample make_truth_sample(double time, const AircraftState& truth)
{
    TrueStateSample sample;

    sample.time = time;
    sample.x = truth.x;
    sample.y = truth.y;
    sample.z = truth.z;
    sample.vx = truth.vx;
    sample.vy = truth.vy;
    sample.vz = truth.vz;
    sample.pitch = truth.pitch;
    sample.roll = truth.roll;
    sample.actual_rpm = truth.actual_rpm;
    sample.actual_left_servo = truth.actual_left_servo;
    sample.actual_right_servo = truth.actual_right_servo;
    return sample;
}

}

/*
Samples the sensor chain (flight-controller view) and the physics ground truth
at the configured interval: simulation-time based, deterministic and
independent from the internal integration tick. Both streams share the same
timestamps so that post-run correlation stays trivial.
*/
void TelemetryRecorder::maybe_record(double time, const AircraftState& truth,
                                     const SensorTelemetry& sensor, const ControlCommand& command,
                                     double commanded_rpm, const TelemetryControl& control)
{
    if (interval_s <= 0.0) {
        return;
    }
    if (time - last_sample_time + 1.0e-9 < interval_s) {
        return;
    }
    last_sample_time = time;

    samples.push_back(make_sensor_sample(time, sensor, command, commanded_rpm, control));
    truth_samples.push_back(make_truth_sample(time, truth));
}

}