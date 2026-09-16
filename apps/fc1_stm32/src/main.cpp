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
import InterFcLink;
import StatusLed;
import ZephyrUartInterFcTransport;

namespace {

FlightCore::Status::StatusLed status_led{};

constexpr std::float64_t kControlPeriodSeconds = 0.01;
constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;
constexpr std::uint32_t kUartReceiveCapacity = 256;
std::int64_t last_remote_status_ms = -1;

constexpr std::int64_t kHeartbeatPeriodMs = 100;
constexpr std::int64_t kLinkReportPeriodMs = 1000;

RING_BUF_DECLARE(uart_receive_buffer, kUartReceiveCapacity);

volatile std::uint16_t inter_fc_next_sequence = 0;
volatile std::uint16_t inter_fc_acknowledged_sequence = 0;
volatile std::uint32_t inter_fc_heartbeat_count = 0;
volatile std::uint32_t inter_fc_acknowledgement_count = 0;
volatile std::uint32_t inter_fc_startup_state = 0;
volatile std::uint32_t inter_fc_heartbeat_suppressed = 0;
volatile std::uint8_t inter_fc_remote_state = static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Unknown);
volatile std::uint8_t inter_fc_remote_detection =
    static_cast<std::uint8_t>(FlightCore::InterFc::DetectionCode::None);

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

/* Sends FC1 heartbeats, receives FC2 acknowledgements, and reports link progress. */
void service_inter_fc_link(FlightCore::InterFc::IInterFcTransport& transport,
                           bool heartbeat_suppressed,
                           std::int64_t& last_heartbeat_ms,
                           std::int64_t& last_report_ms) noexcept
{
    const std::int64_t now_ms = k_uptime_get();
    if (!heartbeat_suppressed && now_ms - last_heartbeat_ms >= kHeartbeatPeriodMs) {
        const FlightCore::InterFc::Message heartbeat{
            .kind = FlightCore::InterFc::MessageKind::Heartbeat,
            .sequence = inter_fc_next_sequence,
            .state = FlightCore::InterFc::NodeState::Healthy,
        };
        if (transport.send(heartbeat).has_value()) {
            inter_fc_heartbeat_count = inter_fc_heartbeat_count + 1;
            inter_fc_next_sequence = static_cast<std::uint16_t>(inter_fc_next_sequence + 1);
        }
        last_heartbeat_ms = now_ms;
    }
    while (true) {
        const auto received = transport.poll();
        if (!received.has_value() || !received->has_value()) {
            break;
        }
        if ((*received)->kind == FlightCore::InterFc::MessageKind::Acknowledgement) {
            inter_fc_acknowledged_sequence = (*received)->sequence;
            inter_fc_acknowledgement_count = inter_fc_acknowledgement_count + 1;
        }
        if ((*received)->kind == FlightCore::InterFc::MessageKind::Acknowledgement
            || (*received)->kind == FlightCore::InterFc::MessageKind::Status) {
            last_remote_status_ms = now_ms;
            inter_fc_remote_state = static_cast<std::uint8_t>((*received)->state);
            inter_fc_remote_detection = static_cast<std::uint8_t>((*received)->detection);
        }
    }
    if (now_ms - last_report_ms >= kLinkReportPeriodMs) {
        printk("[INTERFC] role=FC1 heartbeats=%u acknowledgements=%u last_ack=%u remote_state=%u suppressed=%u\n",
               static_cast<unsigned int>(inter_fc_heartbeat_count),
               static_cast<unsigned int>(inter_fc_acknowledgement_count),
               static_cast<unsigned int>(inter_fc_acknowledged_sequence),
               static_cast<unsigned int>(inter_fc_remote_state),
               static_cast<unsigned int>(inter_fc_heartbeat_suppressed));
        last_report_ms = now_ms;
    }
}

/* Executes one FC1 cycle for a validated HIL SensorPacket. */
void process_sensor(const device* uart,
                    FlightCore::InterFc::IInterFcTransport& inter_fc_transport,
                    sim::control::FlightController& controller,
                    sim::control::TargetState& current_setpoint,
                    const FlightCore::Transport::HilHeader& header,
                    std::span<const std::uint8_t> payload) noexcept
{
    FlightCore::Transport::HilSensorPayload sensor_payload{};
    if (!FlightCore::Transport::decodeSensorPayload(payload, sensor_payload)) {
        return;
    }
    inter_fc_heartbeat_suppressed =
        (sensor_payload.sensor_valid_flags & FlightCore::Transport::kHilCommandSuppressInterFcHeartbeat) != 0
            ? 1u
            : 0u;
    if (header.sequence_num == 0 && sensor_payload.sim_timestamp_us == 0) {
        controller = sim::control::FlightController{
            sim::control::ControllerConfig{.hover_rpm = kNominalAircraftHoverRpm,
                                           .station_hold_seconds = sensor_payload.setpoint.station_hold_seconds}};
        inter_fc_remote_state = static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Unknown);
        inter_fc_remote_detection = static_cast<std::uint8_t>(FlightCore::InterFc::DetectionCode::None);
        const FlightCore::InterFc::Message reset_supervision{
            .kind = FlightCore::InterFc::MessageKind::ResetSupervision,
        };
        static_cast<void>(inter_fc_transport.send(reset_supervision));
    }
    const FlightCore::HAL::SensorData sensor = FlightCore::Transport::toSensorData(sensor_payload);
    const AircraftState measured = to_aircraft_state(sensor);
    current_setpoint = sim::control::TargetState{
        .x = sensor_payload.setpoint.target_x_m,
        .y = sensor_payload.setpoint.target_y_m,
        .z = sensor_payload.setpoint.target_z_m,
    };
    const ControlCommand command = controller.update(current_setpoint, measured, kControlPeriodSeconds);
    const FlightCore::InterFc::Message monitoring_sample{
        .kind = FlightCore::InterFc::MessageKind::MonitoringSample,
        .sequence = header.sequence_num,
        .state = FlightCore::InterFc::NodeState::Healthy,
        .detection = FlightCore::InterFc::DetectionCode::None,
        .altitude_m = sensor.altitude_baro_m,
        .actual_rpm = sensor.wing_rpm_meas,
        .commanded_rpm = static_cast<std::float32_t>(command.wing_rpm),
    };
    static_cast<void>(inter_fc_transport.send(monitoring_sample));
    const FlightCore::HAL::ActuatorCommands actuators{
        .timestamp_us = static_cast<std::uint64_t>(k_uptime_get()) * 1000u,
        .wing_rpm_cmd = static_cast<std::float32_t>(command.wing_rpm),
        .left_servo_rad = static_cast<std::float32_t>(command.left_servo_angle * kRadiansPerDegree),
        .right_servo_rad = static_cast<std::float32_t>(command.right_servo_angle * kRadiansPerDegree),
        .aux_actuator_cmd = 0.0f,
        .mode_flags = static_cast<std::uint8_t>(controller.state()),
    };
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{
        .fc_health_status = inter_fc_remote_state,
        .fc_detection_code = inter_fc_remote_detection,
    };
    const auto response_payload = FlightCore::Transport::makeActuatorPayload(
        actuators, sensor_payload.sim_timestamp_us, diagnostics);
    const auto response = FlightCore::Transport::encodeActuatorFrame(response_payload, header.sequence_num);
    send_frame(uart, response);
    status_led.mark_activity(k_uptime_get());
}

}

/* Zephyr FC1 entry point. Incoming SensorPackets provide the 100 Hz release cadence. */
int main()
{
    const auto led_initialized = status_led.initialize();
    if (!led_initialized.has_value()) {
        printk("[LED] initialization failed: %d\n", led_initialized.error());
    }
    inter_fc_startup_state = 1;
    const device* uart = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    const device* inter_fc_uart = DEVICE_DT_GET(DT_NODELABEL(usart3));
    FlightCore::InterFc::ZephyrUartInterFcTransport inter_fc_transport{inter_fc_uart};
    if (!device_is_ready(uart)) {
        return -1;
    }
    inter_fc_startup_state = 2;
    if (!inter_fc_transport.ready()) {
        return -1;
    }
    inter_fc_startup_state = 3;
    printk("[BOOT] firmware=fc1_stm32 role=FC1 board=nucleo_l476rg hil_baud=460800 inter_fc=USART3/PB10/PB11/115200\n");

    sim::control::ControllerConfig config{.hover_rpm = kNominalAircraftHoverRpm};
    sim::control::FlightController controller{config};
    sim::control::TargetState current_setpoint{};
    FlightCore::Transport::HilFrameParser parser{};
    FlightCore::Transport::HilHeader header{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};
    std::int64_t last_heartbeat_ms = -kHeartbeatPeriodMs;
    std::int64_t last_report_ms = 0;
    if (uart_irq_callback_user_data_set(uart, receive_uart_bytes, nullptr) != 0) {
        return -1;
    }
    uart_irq_rx_enable(uart);
    inter_fc_startup_state = 4;

    while (true) {
        service_inter_fc_link(inter_fc_transport,
                              inter_fc_heartbeat_suppressed != 0,
                              last_heartbeat_ms,
                              last_report_ms);
        const std::int64_t led_now_ms = k_uptime_get();
        const bool remote_missing = last_remote_status_ms < 0
            ? led_now_ms >= 2000 : led_now_ms - last_remote_status_ms >= 500;
        const bool led_fault = remote_missing || inter_fc_heartbeat_suppressed != 0
            || inter_fc_remote_state == static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Degraded)
            || inter_fc_remote_state == static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Safe)
            || inter_fc_remote_detection != static_cast<std::uint8_t>(FlightCore::InterFc::DetectionCode::None);
        static_cast<void>(status_led.update(led_now_ms, led_fault));
        std::uint8_t byte = 0;
        if (ring_buf_get(&uart_receive_buffer, &byte, 1) == 1) {
            const bool complete = parser.processByte(byte, header, payload);
            if (complete && header.msg_id == FlightCore::Transport::kMsgIdSensor
                && header.payload_len == FlightCore::Transport::kSensorPayloadSize) {
                process_sensor(uart, inter_fc_transport, controller, current_setpoint, header,
                    std::span<const std::uint8_t>{payload.data(), header.payload_len});
            }
        }
        else {
            k_sleep(K_MSEC(1));
        }
    }
}
