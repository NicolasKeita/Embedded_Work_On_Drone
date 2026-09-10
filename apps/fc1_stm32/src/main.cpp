/*
Filename: apps/fc1_stm32/src/main.cpp
Description: Zephyr runtime receiving HIL sensors and executing the shared FC1 control core.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/drivers/uart.h>

import std;

import Aircraft;
import FlightController;
import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;

namespace {

constexpr std::float64_t kControlPeriodSeconds = 0.01;
constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;
constexpr std::uint8_t kHealthy = 1;
constexpr std::uint32_t kUartReceiveCapacity = 256;

RING_BUF_DECLARE(uart_receive_buffer, kUartReceiveCapacity);

/* Moves received UART bytes from the hardware FIFO into the static HIL ring buffer. */
void receive_uart_bytes(const device* uart, void*) noexcept
{
    std::array<std::uint8_t, 64> received_bytes{};

    while (uart_irq_update(uart) != 0 && uart_irq_is_pending(uart) != 0) {
        if (uart_irq_rx_ready(uart) == 0) {
            continue;
        }
        const std::int32_t received = uart_fifo_read(uart, received_bytes.data(), received_bytes.size());
        if (received > 0) {
            ring_buf_put(&uart_receive_buffer, received_bytes.data(), static_cast<std::uint32_t>(received));
        }
    }
}

/* Converts the shared HIL sensor contract into the existing FC1 measurement type. */
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

/* Sends one complete frame without allocating or blocking on a buffered stream. */
void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept
{
    for (const std::uint8_t byte : frame) {
        uart_poll_out(uart, byte);
    }
}

/* Executes one FC1 cycle for a validated HIL SensorPacket. */
void process_sensor(const device* uart,
                    sim::control::FlightController& controller,
                    const FlightCore::Transport::HilHeader& header,
                    std::span<const std::uint8_t> payload) noexcept
{
    FlightCore::Transport::HilSensorPayload sensor_payload{};
    if (!FlightCore::Transport::decodeSensorPayload(payload, sensor_payload)) {
        return;
    }
    if (header.sequence_num == 0 && sensor_payload.sim_timestamp_us == 0) {
        controller = sim::control::FlightController{
            sim::control::ControllerConfig{.hover_rpm = kNominalAircraftHoverRpm}};
    }
    const FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const AircraftState measured = to_aircraft_state(sensor);
    const sim::control::TargetState target{.z = 10.0};
    const ControlCommand command = controller.update(target, measured, kControlPeriodSeconds);
    const FlightCore::HAL::ActuatorCommands actuators{
        .timestamp_us = static_cast<std::uint64_t>(k_uptime_get()) * 1000u,
        .wing_rpm_cmd = static_cast<std::float32_t>(command.wing_rpm),
        .left_servo_rad = static_cast<std::float32_t>(command.left_servo_angle * kRadiansPerDegree),
        .right_servo_rad = static_cast<std::float32_t>(command.right_servo_angle * kRadiansPerDegree),
        .aux_actuator_cmd = 0.0f,
        .mode_flags = static_cast<std::uint8_t>(controller.state()),
    };
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{.fc_health_status = kHealthy};
    const auto response_payload = FlightCore::Transport::makeActuatorPayload(
        actuators, sensor_payload.sim_timestamp_us, diagnostics);
    const auto response = FlightCore::Transport::encodeActuatorFrame(response_payload, header.sequence_num);
    send_frame(uart, response);
}

}

/* Zephyr FC1 entry point. Incoming SensorPackets provide the 100 Hz release cadence. */
int main()
{
    const device* uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    if (!device_is_ready(uart)) {
        return -1;
    }
    printk("[BOOT] firmware=fc1_stm32 role=FC1 board=nucleo_l476rg period_us=10000 baud=460800 protocol=1.0\n");

    sim::control::ControllerConfig config{.hover_rpm = kNominalAircraftHoverRpm};
    sim::control::FlightController controller{config};
    FlightCore::Transport::HilFrameParser parser{};
    FlightCore::Transport::HilHeader header{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};
    if (uart_irq_callback_user_data_set(uart, receive_uart_bytes, nullptr) != 0) {
        return -1;
    }
    uart_irq_rx_enable(uart);

    while (true) {
        std::uint8_t byte = 0;
        if (ring_buf_get(&uart_receive_buffer, &byte, 1) == 1) {
            const bool complete = parser.processByte(byte, header, payload);
            if (complete && header.msg_id == FlightCore::Transport::kMsgIdSensor
                && header.payload_len == FlightCore::Transport::kSensorPayloadSize) {
                process_sensor(uart, controller, header,
                    std::span<const std::uint8_t>{payload.data(), header.payload_len});
            }
        }
        else {
            k_sleep(K_MSEC(1));
        }
    }
}
