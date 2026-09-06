/*
Filename: Src/Embedded/Hil/Telemetry/HilTelemetry.cpp
Description: Fixed-rate dual telemetry sampler : builds one truth sample from the
aircraft physics state and one sensor sample from the HAL sensor snapshot the FC
observed, recorded at the configured structured rate only.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTelemetry;

import std;

import Aircraft;
import HalTypes;

namespace sim::hil {

namespace {
    constexpr std::float64_t kDegreesPerRadian = 180.0 / std::numbers::pi;
    constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;
}

HilTruthSample make_truth_sample(std::float64_t time_s, const AircraftState& truth)
{
    return HilTruthSample{
        .time_s = time_s,
        .x = truth.x,
        .y = truth.y,
        .z = truth.z,
        .vx = truth.vx,
        .vy = truth.vy,
        .vz = truth.vz,
        .pitch_rad = truth.pitch,
        .roll_rad = truth.roll,
        .actual_rpm = truth.actual_rpm,
        .actual_left_servo_deg = truth.actual_left_servo,
        .actual_right_servo_deg = truth.actual_right_servo,
    };
}

HilSensorSample make_sensor_sample(std::float64_t                           time_s,
                                   const FlightCore::HAL::SensorData&       sensor,
                                   const FlightCore::HAL::ActuatorCommands& command,
                                   std::float64_t                           commanded_rpm,
                                   const HilTelemetryControl&               control)
{
    return HilSensorSample{
        .time_s = time_s,
        .x = sensor.position_x_m,
        .y = sensor.position_y_m,
        .z = sensor.position_z_m,
        .vx = sensor.velocity_x_ms,
        .vy = sensor.velocity_y_ms,
        .vz = sensor.velocity_z_ms,
        .pitch_rad = sensor.pitch_rad,
        .roll_rad = sensor.roll_rad,
        .commanded_rpm = commanded_rpm,
        .measured_rpm = sensor.wing_rpm_meas,
        .left_servo_deg = command.left_servo_rad / kRadiansPerDegree,
        .right_servo_deg = command.right_servo_rad / kRadiansPerDegree,
        .target_x = control.target_x,
        .target_y = control.target_y,
        .target_z = control.target_z,
        .mission_state = control.mission_state,
        .safety_state = control.safety_state,
    };
}

void HilTelemetryRecorder::maybe_record(std::float64_t                           time_s,
                                        const AircraftState&                     truth,
                                        const FlightCore::HAL::SensorData&       sensor,
                                        const FlightCore::HAL::ActuatorCommands& command,
                                        std::float64_t                           commanded_rpm,
                                        const HilTelemetryControl&               control)
{
    if (time_s - last_sample_time + 1.0e-9 < interval_s) {
        return;
    }
    last_sample_time = time_s;
    truth_samples.push_back(make_truth_sample(time_s, truth));
    samples.push_back(make_sensor_sample(time_s, sensor, command, commanded_rpm, control));
}

}
