/*
Filename: Src/Embedded/Demo/Demo-Sensor.cpp
Description: Simulator-side sensor half of the HIL lockstep demo: builds the
simulated sensor sample from the private ground-truth state, encodes and sends
the SensorPacket, then lets the Flight Controller read it back through
ISensorInput.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Demo;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import Transport;

namespace FlightCore::Demo
{

HAL::SensorData buildSensorSample(std::uint64_t timestamp_us, const TruthState& truth) noexcept
{
    return HAL::SensorData{
        .timestamp_us = timestamp_us,
        .position_x_m = 0.0f,
        .position_y_m = 0.0f,
        .position_z_m = truth.altitude_m + 0.02f,
        .velocity_x_ms = 0.0f,
        .velocity_y_ms = 0.0f,
        .velocity_z_ms = truth.climb_rate_ms,
        .gyro_roll_rad_s = 0.0f,
        .gyro_pitch_rad_s = 0.0f,
        .gyro_yaw_rad_s = 0.0f,
        .accel_x_m_s2 = 0.0f,
        .accel_y_m_s2 = 0.0f,
        .accel_z_m_s2 = 0.0f,
        .roll_rad = 0.01f,
        .pitch_rad = 0.02f,
        .yaw_rad = 0.0f,
        .altitude_baro_m = truth.altitude_m,
        .wing_rpm_meas = 1000.0f,
        .sensor_valid_flags = HAL::kFlagImu1Ok | HAL::kFlagImu2Ok | HAL::kFlagBaroOk
                            | HAL::kFlagGpsFixOk | HAL::kFlagTachometerOk, };
}

/*
    Simulator half of the lockstep: builds the sensor sample from ground truth,
    sends the SensorPacket and lets the Flight Controller decode and consume it
    through ISensorInput.
*/
bool exchangeSensorSample(DemoContext&      context,
                          std::uint16_t     sequence,
                          const TruthState& truth,
                          HAL::SensorData&  out_tx_sensor,
                          HAL::SensorData&  out_fc_sensor)
{
    out_tx_sensor = buildSensorSample(context.clock.nowUs(), truth);

    const auto wire_sensor = Transport::makeSensorPayload(out_tx_sensor);
    const auto sensor_frame = Transport::encodeSensorFrame(wire_sensor, sequence);
    if (!context.transport.sendBytes(sensor_frame)) {
        return false;
    }

    Transport::HilHeader header{};
    if (!receiveFrame(context.transport, context.parser, header, context.payload_buffer)) {
        return false;
    }

    Transport::HilSensorPayload decoded_sensor{};
    const std::span<const std::uint8_t> payload(context.payload_buffer.data(), header.payload_len);
    if (!Transport::decodeSensorPayload(payload, decoded_sensor)) {
        return false;
    }

    out_fc_sensor = Transport::toSensorData(decoded_sensor);
    context.sensor_input.inject(out_fc_sensor);
    return context.sensor_input.readSensorData(out_fc_sensor);
}

}
