/*
Filename: Src/SIL/Core/Telemetry/SilTelemetry.cppm
Description: Structured sensor/ground-truth telemetry streams and fixed-rate dual sampler.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilTelemetry;

import std;

import Aircraft;
import Telemetry;

export namespace sim::sil {

/*
One sampled sensor telemetry record: values come from the simulated sensor
chain, i.e. exactly what the flight controller observes, never from the physics
ground truth. Control fields are enum values (see mission_state_name and
safety_mode_name for the textual form).
*/
struct TelemetrySample {
    double time = 0.0;
    double x = 0.0;
    double y = 0.0;
    double altitude_m = 0.0;
    double pitch_rad = 0.0;
    double roll_rad = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double vz = 0.0;
    double commanded_rpm = 0.0;
    double actual_rpm = 0.0;
    double left_servo_deg = 0.0;
    double right_servo_deg = 0.0;
    double target_x = 0.0;
    double target_y = 0.0;
    double target_z = 0.0;
    int    mission_state = 0;
    int    safety_state = 0;
};

/*
One sampled physics ground-truth record, kept in a separate stream for
simulator validation; never mixed with the sensor telemetry samples.
*/
struct TrueStateSample {
    double time = 0.0;
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

// Control context captured with each telemetry sample.
struct TelemetryControl {
    double target_x = 0.0;
    double target_y = 0.0;
    double target_z = 0.0;
    int    mission_state = 0;
    int    safety_state = 0;
};

/*
Fixed-rate dual sampler: the sensor path feeds the TelemetrySample stream (what
the FC observes) while the physics state feeds the parallel TrueStateSample
stream (ground truth). Rate is configured through interval_s.
*/
struct TelemetryRecorder {
    double                       interval_s = 0.05;
    double                       last_sample_time = -1.0e12;
    std::vector<TelemetrySample> samples;
    std::vector<TrueStateSample> truth_samples;

    void maybe_record(double time, const AircraftState& truth, const SensorTelemetry& sensor,
                      const ControlCommand& command, double commanded_rpm,
                      const TelemetryControl& control);
};

}
