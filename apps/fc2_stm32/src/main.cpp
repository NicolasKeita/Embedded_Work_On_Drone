/*
Filename: apps/fc2_stm32/src/main.cpp
Description: Zephyr runtime executing the shared FC2 health and safety supervision core.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>

import std;

import CommsBus;
import HalTypes;
import HealthMonitor;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import SafetyManager;
import Telemetry;

namespace {

/* Converts a HIL sensor packet into the telemetry consumed by the FC2 core. */
sim::sil::SensorTelemetry to_telemetry(const FlightCore::HAL::SensorData& sensor) noexcept
{
    return sim::sil::SensorTelemetry{
        .x = sensor.position_x_m,
        .y = sensor.position_y_m,
        .z = sensor.altitude_baro_m,
        .vx = sensor.velocity_x_ms,
        .vy = sensor.velocity_y_ms,
        .vz = sensor.velocity_z_ms,
        .pitch = sensor.pitch_rad,
        .roll = sensor.roll_rad,
        .actual_rpm = sensor.wing_rpm_meas,
    };
}

/* Sends one complete frame through the selected Zephyr console UART. */
void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept
{
    for (const std::uint8_t byte : frame) {
        uart_poll_out(uart, byte);
    }
}

/* Runs FC2 health evaluation and returns its safety decision as a HIL response. */
void process_sensor(const device* uart,
                    sim::safety::LinkSupervision& supervision,
                    sim::safety::HealthMonitor& monitor,
                    sim::safety::SafetyManager& safety,
                    const FlightCore::Transport::HilHeader& header,
                    std::span<const std::uint8_t> payload) noexcept
{
    FlightCore::Transport::HilSensorPayload sensor_payload{};
    if (!FlightCore::Transport::decodeSensorPayload(payload, sensor_payload)) {
        return;
    }
    const std::float64_t now = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
    supervision.last_heartbeat_time = now;
    const auto sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const auto telemetry = to_telemetry(sensor);
    const auto report = monitor.evaluate(now, supervision, telemetry, sensor.wing_rpm_meas);
    const auto command = safety.update(now, report);
    const FlightCore::HAL::ActuatorCommands response_command{
        .timestamp_us = static_cast<std::uint64_t>(k_uptime_get()) * 1000u,
        .wing_rpm_cmd = 0.0f,
        .left_servo_rad = 0.0f,
        .right_servo_rad = 0.0f,
        .aux_actuator_cmd = static_cast<std::float32_t>(command.thrust_margin),
        .mode_flags = static_cast<std::uint8_t>(safety.mode()),
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

/* Zephyr FC2 entry point. SensorPackets act as the initial supervision heartbeat. */
int main()
{
    const device* uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    if (!device_is_ready(uart)) {
        return -1;
    }
    printk("[BOOT] firmware=fc2_stm32 role=FC2 board=nucleo_l476rg period_us=10000 protocol=1.0\n");

    sim::safety::LinkSupervision supervision{};
    sim::safety::HealthMonitor monitor{};
    sim::safety::SafetyManager safety{};
    FlightCore::Transport::HilFrameParser parser{};
    FlightCore::Transport::HilHeader header{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};

    while (true) {
        std::uint8_t byte = 0;
        if (uart_poll_in(uart, &byte) == 0) {
            const bool complete = parser.processByte(byte, header, payload);
            if (complete && header.msg_id == FlightCore::Transport::kMsgIdSensor
                && header.payload_len == FlightCore::Transport::kSensorPayloadSize) {
                process_sensor(uart, supervision, monitor, safety, header,
                    std::span<const std::uint8_t>{payload.data(), header.payload_len});
            }
        }
        else {
            k_sleep(K_MSEC(1));
        }
    }
}
