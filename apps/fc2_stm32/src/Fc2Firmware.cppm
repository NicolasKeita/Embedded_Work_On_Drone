/*
Filename: apps/fc2_stm32/src/Fc2Firmware.cppm
Description: Interface of the Zephyr FC2 firmware: shared link bookkeeping, the health and
safety supervision session bundles, and the internal helpers implemented by the
Fc2Firmware-*.cpp units. The exported entry point run_fc2_firmware() runs the main loop.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/kernel.h>

export module Fc2Firmware;

import std;

import HalTypes;
import HealthMonitor;
import HilProtocol;
import InterFcLink;
import SafetyManager;
import StatusLed;
import Telemetry;
import ZephyrUartInterFcTransport;

namespace fc2 {

constexpr std::int64_t kLinkReportPeriodMs = 1000;
constexpr std::int64_t kStatusPeriodMs = 100;
constexpr std::float64_t kHeartbeatTimeoutSeconds = 0.30;
constexpr std::float64_t kInitialHeartbeatTimeoutSeconds = 2.0;

/* Volatile link bookkeeping shared by every FC2 firmware unit. */
struct Fc2Link {
    volatile std::uint16_t last_sequence = 0;
    volatile std::uint32_t heartbeat_count = 0;
    volatile std::uint32_t acknowledgement_count = 0;
    volatile std::uint32_t startup_state = 0;
    volatile std::uint8_t  node_state = static_cast<std::uint8_t>(FlightCore::InterFc::NodeState::Unknown);
};

extern Fc2Link inter_fc;
extern FlightCore::Status::StatusLed status_led;

/* Hardware handles of a booted FC2 unit. */
struct Fc2Boot {
    FlightCore::InterFc::ZephyrUartInterFcTransport inter_fc_transport{nullptr};
    const device*                                   uart = nullptr;
    bool                                            ready = false;
};

/* Configures the HIL console UART and the inter-FC USART of the FC2 unit. */
Fc2Boot boot_fc2_hardware();

/* Advances the status LED from the current supervision state. */
void update_status_led(FlightCore::InterFc::NodeState node_state,
                      FlightCore::InterFc::DetectionCode detection) noexcept;

/*
Health and safety supervision state of one FC2 run: the monitored link, the shared
HealthMonitor/SafetyManager cores, the monitoring sample and the supervision reset.
*/
struct Fc2Health {
    sim::safety::LinkSupervision& supervision;
    sim::safety::HealthMonitor&   monitor;
    sim::safety::SafetyManager&   safety;
    sim::sil::SensorTelemetry&    telemetry;
    std::float64_t&               commanded_rpm;
    bool&                         reset_requested;
};

/* Sends one complete frame through the selected Zephyr console UART. */
void send_frame(const device* uart, std::span<const std::uint8_t> frame) noexcept;

/* Maps the safety mode to the inter-FC node state vocabulary. */
FlightCore::InterFc::NodeState to_node_state(sim::safety::SafetyMode mode) noexcept;

/* Maps the first active detection to the inter-FC diagnostic vocabulary. */
FlightCore::InterFc::DetectionCode to_detection_code(const sim::safety::HealthReport& report) noexcept;

/* Converts a HIL sensor packet into the telemetry consumed by the FC2 core. */
sim::sil::SensorTelemetry to_telemetry(const FlightCore::HAL::SensorData& sensor) noexcept;

/* Restarts the health and safety supervision when a new run resets the session. */
void reset_supervision(Fc2Health& health) noexcept;

/* Publishes FC2 health periodically, including while FC1 heartbeats are absent. */
void publish_fc2_status(FlightCore::InterFc::IInterFcTransport& transport,
                        FlightCore::InterFc::NodeState node_state,
                        FlightCore::InterFc::DetectionCode detection,
                        std::int64_t& last_status_ms) noexcept;

/* Reports FC2 raw-byte and validated-frame diagnostics for the USART3 adapter. */
void report_inter_fc_transport(const FlightCore::InterFc::ZephyrUartInterFcTransport& transport,
                               std::int64_t& last_transport_report_ms) noexcept;

/* Evaluates FC1 liveness continuously and applies the FC2 safety decision. */
FlightCore::InterFc::NodeState evaluate_fc1_health(Fc2Health& health,
                                                    FlightCore::InterFc::DetectionCode& detection) noexcept;

/* Receives FC1 heartbeats, acknowledges them, and refreshes FC2 link supervision. */
void service_inter_fc_link(FlightCore::InterFc::IInterFcTransport& transport,
                            Fc2Health& health,
                            FlightCore::InterFc::NodeState node_state,
                            FlightCore::InterFc::DetectionCode detection,
                            std::int64_t& last_report_ms) noexcept;

/* Runs FC2 health evaluation and returns its safety decision as a HIL response. */
void process_sensor(const device* uart,
                    Fc2Health& health,
                    const FlightCore::Transport::HilHeader& header,
                    std::span<const std::uint8_t> payload) noexcept;

}

/* Zephyr FC2 entry point. SensorPackets act as the initial supervision heartbeat. */
export int run_fc2_firmware();
