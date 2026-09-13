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
import InterFcLink;
import SafetyManager;
import Telemetry;
import ZephyrUartInterFcTransport;

namespace {

constexpr std::int64_t kLinkReportPeriodMs = 1000;
constexpr std::int64_t kStatusPeriodMs = 100;
constexpr std::float64_t kHeartbeatTimeoutSeconds = 0.30;
constexpr std::float64_t kInitialHeartbeatTimeoutSeconds = 2.0;

volatile std::uint16_t inter_fc_last_sequence = 0;
volatile std::uint32_t inter_fc_heartbeat_count = 0;
volatile std::uint32_t inter_fc_acknowledgement_count = 0;
volatile std::uint32_t inter_fc_startup_state = 0;
volatile std::uint8_t inter_fc_node_state = static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Unknown);

/* Maps the safety manager state to the transport-independent FC-to-FC status. */
FlightCore::InterFc::NodeState to_node_state(sim::safety::SafetyMode mode) noexcept
{
    switch (mode) {
    case sim::safety::SafetyMode::NORMAL:
        return FlightCore::InterFc::NodeState::Healthy;
    case sim::safety::SafetyMode::COMPENSATED:
        return FlightCore::InterFc::NodeState::Degraded;
    case sim::safety::SafetyMode::SAFE_MODE:
        return FlightCore::InterFc::NodeState::Safe;
    }
    return FlightCore::InterFc::NodeState::Unknown;
}

/* Maps the first active FC2 detection to the inter-FC diagnostic vocabulary. */
FlightCore::InterFc::DetectionCode to_detection_code(const sim::safety::HealthReport& report) noexcept
{
    for (std::size_t index = 0; index < report.flags.size(); ++index) {
        if (!report.flags[index].raised) {
            continue;
        }
        switch (static_cast<sim::safety::DetectionEvent>(index)) {
        case sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT:
            return FlightCore::InterFc::DetectionCode::Fc1HeartbeatTimeout;
        case sim::safety::DetectionEvent::COMMUNICATION_TIMEOUT:
            return FlightCore::InterFc::DetectionCode::CommunicationTimeout;
        case sim::safety::DetectionEvent::SENSOR_VALIDATION_FAILED:
            return FlightCore::InterFc::DetectionCode::SensorValidationFailed;
        case sim::safety::DetectionEvent::ACTUATOR_MISMATCH:
            return FlightCore::InterFc::DetectionCode::ActuatorMismatch;
        }
    }
    return FlightCore::InterFc::DetectionCode::None;
}

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

/* Receives FC1 heartbeats, acknowledges them, and refreshes FC2 link supervision. */
void service_inter_fc_link(FlightCore::InterFc::IInterFcTransport& transport,
                           sim::safety::LinkSupervision& supervision,
                           sim::sil::SensorTelemetry& telemetry,
                           std::float64_t& commanded_rpm,
                           FlightCore::InterFc::NodeState node_state,
                           FlightCore::InterFc::DetectionCode detection,
                           bool& reset_requested,
                           std::int64_t& last_report_ms) noexcept
{
    while (true) {
        const auto received = transport.poll();
        if (!received.has_value() || !received->has_value()) {
            break;
        }
        if ((*received)->kind == FlightCore::InterFc::MessageKind::ResetSupervision) {
            reset_requested = true;
            continue;
        }
        if ((*received)->kind == FlightCore::InterFc::MessageKind::MonitoringSample) {
            telemetry.z = (*received)->altitude_m;
            telemetry.actual_rpm = (*received)->actual_rpm;
            commanded_rpm = (*received)->commanded_rpm;
            continue;
        }
        if ((*received)->kind != FlightCore::InterFc::MessageKind::Heartbeat) {
            continue;
        }
        inter_fc_last_sequence = (*received)->sequence;
        inter_fc_heartbeat_count = inter_fc_heartbeat_count + 1;
        supervision.last_heartbeat_time = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
        const FlightCore::InterFc::Message acknowledgement{
            .kind = FlightCore::InterFc::MessageKind::Acknowledgement,
            .sequence = inter_fc_last_sequence,
            .state = node_state,
            .detection = detection,
        };
        if (transport.send(acknowledgement).has_value()) {
            inter_fc_acknowledgement_count = inter_fc_acknowledgement_count + 1;
        }
    }
    const std::int64_t now_ms = k_uptime_get();
    if (now_ms - last_report_ms >= kLinkReportPeriodMs) {
        printk("[INTERFC] role=FC2 heartbeats=%u acknowledgements=%u last_heartbeat=%u\n",
               static_cast<unsigned int>(inter_fc_heartbeat_count),
               static_cast<unsigned int>(inter_fc_acknowledgement_count),
               static_cast<unsigned int>(inter_fc_last_sequence));
        last_report_ms = now_ms;
    }
}

/* Evaluates FC1 liveness continuously and applies the FC2 safety decision. */
FlightCore::InterFc::NodeState evaluate_fc1_health(sim::safety::LinkSupervision& supervision,
                                                    sim::safety::HealthMonitor& monitor,
                                                    sim::safety::SafetyManager& safety,
                                                    const sim::sil::SensorTelemetry& telemetry,
                                                    std::float64_t commanded_rpm,
                                                    FlightCore::InterFc::DetectionCode& detection) noexcept
{
    const std::float64_t now = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
    const sim::safety::SafetyMode previous_mode = safety.mode();
    const sim::safety::HealthReport report = monitor.evaluate(now, supervision, telemetry, commanded_rpm);
    static_cast<void>(safety.update(now, report));
    const FlightCore::InterFc::NodeState node_state = to_node_state(safety.mode());
    inter_fc_node_state = static_cast<std::uint8_t>(node_state);
    detection = to_detection_code(report);
    if (safety.mode() != previous_mode) {
        printk("[HEALTH] role=FC2 health=%s safety=%s heartbeat_age_ms=%d\n",
               sim::safety::health_state_name(report.state).data(),
               sim::safety::safety_mode_name(safety.mode()).data(),
               supervision.last_heartbeat_time >= 0.0
                   ? static_cast<int>((now - supervision.last_heartbeat_time) * 1000.0)
                   : -1);
    }
    return node_state;
}

/* Publishes FC2 health periodically, including while FC1 heartbeats are absent. */
void publish_fc2_status(FlightCore::InterFc::IInterFcTransport& transport,
                        FlightCore::InterFc::NodeState node_state,
                        FlightCore::InterFc::DetectionCode detection,
                        std::int64_t& last_status_ms) noexcept
{
    const std::int64_t now_ms = k_uptime_get();
    if (now_ms - last_status_ms < kStatusPeriodMs) {
        return;
    }
    const FlightCore::InterFc::Message status{
        .kind = FlightCore::InterFc::MessageKind::Status,
        .sequence = inter_fc_last_sequence,
        .state = node_state,
        .detection = detection,
    };
    static_cast<void>(transport.send(status));
    last_status_ms = now_ms;
}

/* Reports FC2 raw-byte and validated-frame diagnostics for the USART3 adapter. */
void report_inter_fc_transport(const FlightCore::InterFc::ZephyrUartInterFcTransport& transport,
                               std::int64_t& last_transport_report_ms) noexcept
{
    const std::int64_t now_ms = k_uptime_get();
    if (now_ms - last_transport_report_ms >= kLinkReportPeriodMs) {
        printk("[INTERFC-RX] role=FC2 bytes=%u valid_frames=%u\n",
               static_cast<unsigned int>(transport.received_byte_count()),
               static_cast<unsigned int>(transport.valid_frame_count()));
        last_transport_report_ms = now_ms;
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
    printk("[BOOT] firmware=fc2_stm32 role=FC2 board=nucleo_l476rg hil_baud=115200 inter_fc=USART3/PB10/PB11/115200\n");

    const std::float64_t monitoring_started_time = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
    sim::safety::LinkSupervision supervision{
        .link_up = true,
        .last_heartbeat_time = -1.0,
        .monitoring_started_time = monitoring_started_time,
    };
    sim::safety::HealthMonitor monitor{sim::safety::HealthMonitorConfig{
        .heartbeat_timeout_s = kHeartbeatTimeoutSeconds,
        .initial_heartbeat_timeout_s = kInitialHeartbeatTimeoutSeconds,
    }};
    sim::safety::SafetyManager safety{};
    FlightCore::Transport::HilFrameParser parser{};
    FlightCore::Transport::HilHeader header{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};
    std::int64_t last_report_ms = 0;
    std::int64_t last_transport_report_ms = 0;
    std::int64_t last_status_ms = -kStatusPeriodMs;
    FlightCore::InterFc::NodeState node_state = FlightCore::InterFc::NodeState::Healthy;
    FlightCore::InterFc::DetectionCode detection = FlightCore::InterFc::DetectionCode::None;
    sim::sil::SensorTelemetry monitored_telemetry{};
    std::float64_t monitored_commanded_rpm = 0.0;
    bool reset_requested = false;
    inter_fc_startup_state = 4;

    while (true) {
        service_inter_fc_link(inter_fc_transport,
                              supervision,
                              monitored_telemetry,
                              monitored_commanded_rpm,
                              node_state,
                              detection,
                              reset_requested,
                              last_report_ms);
        if (reset_requested) {
            monitor = sim::safety::HealthMonitor{sim::safety::HealthMonitorConfig{
                .heartbeat_timeout_s = kHeartbeatTimeoutSeconds,
                .initial_heartbeat_timeout_s = kInitialHeartbeatTimeoutSeconds,
            }};
            sim::safety::rearm_link_supervision(
                supervision, static_cast<std::float64_t>(k_uptime_get()) / 1000.0);
            safety = sim::safety::SafetyManager{};
            node_state = FlightCore::InterFc::NodeState::Healthy;
            detection = FlightCore::InterFc::DetectionCode::None;
            monitored_telemetry = sim::sil::SensorTelemetry{};
            monitored_commanded_rpm = 0.0;
            reset_requested = false;
        }
        node_state = evaluate_fc1_health(supervision,
                                         monitor,
                                         safety,
                                         monitored_telemetry,
                                         monitored_commanded_rpm,
                                         detection);
        publish_fc2_status(inter_fc_transport, node_state, detection, last_status_ms);
        report_inter_fc_transport(inter_fc_transport, last_transport_report_ms);
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
