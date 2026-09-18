/*
Filename: apps/fc1_stm32/src/Fc1Firmware.cppm
Description: Interface of the Zephyr FC1 firmware: shared inter-FC link bookkeeping, the
hardware-channel and control-session bundles, and the internal helpers implemented by the
Fc1Firmware-*.cpp units. The exported entry point run_fc1_firmware() runs the main loop.

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

export module Fc1Firmware;

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

namespace fc1 {

constexpr std::float64_t kControlPeriodSeconds = 0.01;
constexpr std::float64_t kRadiansPerDegree = std::numbers::pi / 180.0;
constexpr std::uint32_t kUartReceiveCapacity = 256;
constexpr std::int64_t kHeartbeatPeriodMs = 100;
constexpr std::int64_t kLinkReportPeriodMs = 1000;

/* Volatile inter-FC link bookkeeping shared by every FC1 firmware unit. */
struct Fc1InterFc {
    volatile std::int64_t  last_remote_status_ms = -1;
    volatile std::uint16_t next_sequence = 0;
    volatile std::uint16_t acknowledged_sequence = 0;
    volatile std::uint32_t heartbeat_count = 0;
    volatile std::uint32_t acknowledgement_count = 0;
    volatile std::uint32_t startup_state = 0;
    volatile std::uint32_t heartbeat_suppressed = 0;
    volatile std::uint8_t  remote_state = 0;
    volatile std::uint8_t  remote_detection = 0;
};

extern Fc1InterFc inter_fc;
extern FlightCore::Status::StatusLed status_led;

/* Hardware channels of one FC1 unit: HIL console UART and inter-FC USART. */
struct Fc1Links {
    const device*                           uart = nullptr;
    FlightCore::InterFc::IInterFcTransport* inter_fc_transport = nullptr;
};

/* Per-run control and HIL parsing state of one FC1 unit. */
struct Fc1Control {
    sim::control::FlightController controller{sim::control::ControllerConfig{
        .hover_rpm = kNominalAircraftHoverRpm}};
    sim::control::TargetState                                    current_setpoint{};
    FlightCore::Transport::HilFrameParser                        parser{};
    FlightCore::Transport::HilHeader                             header{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};
};

/* Sends one complete frame without allocating or blocking on a buffered stream. */
void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept;

/* Restarts the controller when a new run resets the session (sequence 0, timestamp 0). */
void reset_controller(const Fc1Links& links,
                      Fc1Control& control,
                      std::float32_t station_hold_seconds) noexcept;

/* Publishes the monitoring sample of one control cycle to the FC2 supervisor. */
void publish_monitoring_sample(const Fc1Links& links,
                               const FlightCore::Transport::HilHeader& header,
                               const FlightCore::HAL::SensorData& sensor,
                               const ControlCommand& command) noexcept;

/* Builds the actuator packet of one control cycle from the FC1 command. */
FlightCore::HAL::ActuatorCommands build_actuator_packet(const ControlCommand& command,
                                                        std::uint8_t mode_flags) noexcept;

/* Sends the actuator answer of one control cycle on the HIL console UART. */
void send_actuator_response(const Fc1Links& links,
                            const FlightCore::Transport::HilHeader& header,
                            const FlightCore::HAL::ActuatorCommands& actuators,
                            std::uint64_t sim_timestamp_us) noexcept;

/* Sends FC1 heartbeats, receives FC2 acknowledgements, and reports link progress. */
void service_inter_fc_link(const Fc1Links& links,
                           bool heartbeat_suppressed,
                           std::int64_t& last_heartbeat_ms,
                           std::int64_t& last_report_ms) noexcept;

/* Executes one FC1 cycle for a validated HIL SensorPacket of the current session. */
void process_sensor(const Fc1Links& links, Fc1Control& control) noexcept;

/* Advances the status LED from the current inter-FC supervision state. */
void update_status_led() noexcept;

}

/* Zephyr FC1 entry point. Incoming SensorPackets provide the 100 Hz release cadence. */
export int run_fc1_firmware();
