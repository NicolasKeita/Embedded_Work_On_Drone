/*
Filename: apps/fc2_stm32/src/Fc2Firmware-Process.cpp
Description: FC2 HIL sensor processing: runs the health evaluation and returns its safety
decision as a HIL actuator response on the console UART.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

module Fc2Firmware;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolCodec;

namespace fc2 {

void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept
{
    for (const std::uint8_t byte : frame) {
        uart_poll_out(uart, byte);
    }
}

void process_sensor(const device*                           uart,
                    Fc2Health&                              health,
                    const FlightCore::Transport::HilHeader& header,
                    std::span<const std::uint8_t>           payload) noexcept
{
    FlightCore::Transport::HilSensorPayload sensor_payload{};

    if (!FlightCore::Transport::decodeSensorPayload(payload, sensor_payload)) {
        return;
    }
    const std::float64_t now = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
    const auto sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const auto telemetry = to_telemetry(sensor);
    const auto report = health.monitor.evaluate(now, health.supervision, telemetry, sensor.wing_rpm_meas);
    const auto command = health.safety.update(now, report);
    const FlightCore::HAL::ActuatorCommands response_command{
        .timestamp_us = static_cast<std::uint64_t>(k_uptime_get()) * 1000u,
        .wing_rpm_cmd = 0.0f,
        .left_servo_rad = 0.0f,
        .right_servo_rad = 0.0f,
        .aux_actuator_cmd = static_cast<std::float32_t>(command.thrust_margin),
        .mode_flags = static_cast<std::uint8_t>(health.safety.mode()),
    };
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{
        .fc_health_status = static_cast<std::uint8_t>(report.state),
    };
    const auto response_payload = FlightCore::Transport::makeActuatorPayload(
        response_command, sensor_payload.sim_timestamp_us, diagnostics);
    const auto response = FlightCore::Transport::encodeActuatorFrame(response_payload, header.sequence_num);
    send_frame(uart, response);
}

}
