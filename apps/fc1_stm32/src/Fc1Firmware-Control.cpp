/*
Filename: apps/fc1_stm32/src/Fc1Firmware-Control.cpp
Description: FC1 control cycle: sensor conversion, controller restart on a new run and
the control update executed for each validated HIL SensorPacket.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

module Fc1Firmware;

import std;

import Aircraft;
import FlightController;
import HalTypes;
import HilProtocol;
import InterFcLink;

namespace fc1 {

namespace {
    /* Converts the shared HIL sensor contract into the FC1 measurement type. */
    AircraftState to_aircraft_state(const FlightCore::HAL::SensorData& sensor) noexcept
    {
        return AircraftState{
            .x = sensor.position_x_m,
            .y = sensor.position_y_m,
            .z = sensor.altitude_baro_m,
            .vx = sensor.velocity_x_ms,
            .vy = sensor.velocity_y_ms,
            .vz = sensor.velocity_z_ms,
            .pitch = sensor.pitch_rad,
            .roll = sensor.roll_rad,
            .pitch_rate = sensor.gyro_pitch_rad_s,
            .roll_rate = sensor.gyro_roll_rad_s,
            .actual_rpm = sensor.wing_rpm_meas,
        };
    }
}

void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept
{
    for (const std::uint8_t byte : frame) {
        uart_poll_out(uart, byte);
    }
}

void reset_controller(const Fc1Links& links,
                      Fc1Control&     control,
                      std::float32_t  station_hold_seconds) noexcept
{
    control.controller = sim::control::FlightController{
        sim::control::ControllerConfig{.hover_rpm = kNominalAircraftHoverRpm,
                                       .station_hold_seconds = station_hold_seconds}};
    inter_fc.remote_state = static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Unknown);
    inter_fc.remote_detection = static_cast<std::uint8_t>(FlightCore::InterFc::DetectionCode::None);
    const FlightCore::InterFc::Message reset_supervision{ .kind = FlightCore::InterFc::MessageKind::ResetSupervision, };
    static_cast<void>(links.inter_fc_transport->send(reset_supervision));
}

void process_sensor(const Fc1Links& links, Fc1Control& control) noexcept
{
    const FlightCore::Transport::HilHeader& header = control.header;

    if (header.msg_id != FlightCore::Transport::kMsgIdSensor
        || header.payload_len != FlightCore::Transport::kSensorPayloadSize) {
        return;
    }
    const std::span<const std::uint8_t> payload{control.payload.data(), header.payload_len};
    FlightCore::Transport::HilSensorPayload sensor_payload{};
    if (!FlightCore::Transport::decodeSensorPayload(payload, sensor_payload)) {
        return;
    }
    inter_fc.heartbeat_suppressed =
        (sensor_payload.sensor_valid_flags & FlightCore::Transport::kHilCommandSuppressInterFcHeartbeat) != 0
            ? 1u : 0u;
    if (header.sequence_num == 0 && sensor_payload.sim_timestamp_us == 0) {
        reset_controller(links, control, sensor_payload.setpoint.station_hold_seconds);
    }
    const FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const AircraftState measured = to_aircraft_state(sensor);
    control.current_setpoint = sim::control::TargetState{
        .x = sensor_payload.setpoint.target_x_m,
        .y = sensor_payload.setpoint.target_y_m,
        .z = sensor_payload.setpoint.target_z_m,
    };
    const ControlCommand command = control.controller.update(control.current_setpoint, measured, kControlPeriodSeconds);
    publish_monitoring_sample(links, header, sensor, command);
    const std::uint8_t mode_flags = static_cast<std::uint8_t>(control.controller.state());
    const FlightCore::HAL::ActuatorCommands actuators = build_actuator_packet(command, mode_flags);
    send_actuator_response(links, header, actuators, sensor_payload.sim_timestamp_us);
}

}
