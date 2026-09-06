/*
Filename: Src/Embedded/Hil/HilSensorModel.cpp
Description: Sampling, wire expansion and view-collapse helpers of the HIL sensor chain.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilSensorModel;

import std;

import Aircraft;
import HalTypes;
import Telemetry;

namespace sim::hil {

HilSensorModel::HilSensorModel(std::float64_t noise_stddev, std::uint64_t seed)
    : generator_{seed}, distribution_{0.0, std::max(std::float64_t{0.0}, noise_stddev)}
{
}

sim::sil::SensorTelemetry HilSensorModel::sample(const AircraftState& truth)
{
    sim::sil::SensorTelemetry sensors = sim::sil::make_telemetry(truth);
    if (distribution_.stddev() <= 0.0) {
        return sensors;
    }
    sensors.x += distribution_(generator_);
    sensors.y += distribution_(generator_);
    sensors.z += distribution_(generator_);
    sensors.vx += distribution_(generator_);
    sensors.vy += distribution_(generator_);
    sensors.vz += distribution_(generator_);
    sensors.pitch += distribution_(generator_);
    sensors.roll += distribution_(generator_);
    return sensors;
}

FlightCore::HAL::SensorData to_sensor_data(const sim::sil::SensorTelemetry& sensors,
                                            std::uint64_t sim_timestamp_us,
                                            const AircraftState& truth,
                                            const sim::sil::SensorValidity& validity) noexcept
{
    std::uint32_t flags = FlightCore::HAL::kFlagImu1Ok | FlightCore::HAL::kFlagImu2Ok
                        | FlightCore::HAL::kFlagTachometerOk;
    if (validity.altitude_valid) {
        flags |= FlightCore::HAL::kFlagBaroOk;
    }
    if (validity.position_valid) {
        flags |= FlightCore::HAL::kFlagGpsFixOk;
    }

    return FlightCore::HAL::SensorData{
        .timestamp_us = sim_timestamp_us,
        .position_x_m = static_cast<std::float32_t>(sensors.x),
        .position_y_m = static_cast<std::float32_t>(sensors.y),
        .position_z_m = static_cast<std::float32_t>(sensors.z),
        .velocity_x_ms = static_cast<std::float32_t>(sensors.vx),
        .velocity_y_ms = static_cast<std::float32_t>(sensors.vy),
        .velocity_z_ms = static_cast<std::float32_t>(sensors.vz),
        .gyro_roll_rad_s = static_cast<std::float32_t>(truth.roll_rate),
        .gyro_pitch_rad_s = static_cast<std::float32_t>(truth.pitch_rate),
        .gyro_yaw_rad_s = 0.0f,
        .accel_x_m_s2 = 0.0f,
        .accel_y_m_s2 = 0.0f,
        .accel_z_m_s2 = 0.0f,
        .roll_rad = static_cast<std::float32_t>(sensors.roll),
        .pitch_rad = static_cast<std::float32_t>(sensors.pitch),
        .yaw_rad = 0.0f,
        .altitude_baro_m = static_cast<std::float32_t>(sensors.z),
        .wing_rpm_meas = static_cast<std::float32_t>(sensors.actual_rpm),
        .sensor_valid_flags = flags,
    };
}

sim::sil::SensorTelemetry to_telemetry(const FlightCore::HAL::SensorData& sensor) noexcept
{
    return sim::sil::SensorTelemetry{
        .x = sensor.position_x_m,
        .y = sensor.position_y_m,
        .z = sensor.position_z_m,
        .vx = sensor.velocity_x_ms,
        .vy = sensor.velocity_y_ms,
        .vz = sensor.velocity_z_ms,
        .pitch = sensor.pitch_rad,
        .roll = sensor.roll_rad,
        .actual_rpm = sensor.wing_rpm_meas,
        .actual_left_servo = 0.0,
        .actual_right_servo = 0.0,
    };
}

AircraftState to_aircraft_state(const FlightCore::HAL::SensorData& sensor) noexcept
{
    AircraftState state{};
    state.x = sensor.position_x_m;
    state.y = sensor.position_y_m;
    state.z = sensor.position_z_m;
    state.vx = sensor.velocity_x_ms;
    state.vy = sensor.velocity_y_ms;
    state.vz = sensor.velocity_z_ms;
    state.pitch = sensor.pitch_rad;
    state.roll = sensor.roll_rad;
    state.actual_rpm = sensor.wing_rpm_meas;
    return state;
}

}
