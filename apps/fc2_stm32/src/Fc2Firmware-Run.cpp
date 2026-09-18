/*
Filename: apps/fc2_stm32/src/Fc2Firmware-Run.cpp
Description: FC2 Zephyr entry point and main supervision loop over the HIL console UART.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

module Fc2Firmware;

import std;

import HealthMonitor;
import HilProtocol;
import HilProtocolParser;
import InterFcLink;
import SafetyManager;
import Telemetry;

namespace fc2 {

namespace {
    /* Run-local health, safety and HIL parsing state of the supervision loop. */
    struct Fc2Session {
        sim::safety::LinkSupervision supervision{
            .link_up = true,
            .last_heartbeat_time = -1.0,
            .monitoring_started_time = static_cast<std::float64_t>(k_uptime_get()) / 1000.0,
        };
        sim::safety::HealthMonitor monitor{sim::safety::HealthMonitorConfig{
            .heartbeat_timeout_s = kHeartbeatTimeoutSeconds,
            .initial_heartbeat_timeout_s = kInitialHeartbeatTimeoutSeconds,
        }};
        sim::safety::SafetyManager safety{};
        sim::sil::SensorTelemetry telemetry{};
        std::float64_t commanded_rpm = 0.0;
        bool reset_requested = false;
        Fc2Health health{.supervision = supervision,
                         .monitor = monitor,
                         .safety = safety,
                         .telemetry = telemetry,
                         .commanded_rpm = commanded_rpm,
                         .reset_requested = reset_requested};
        FlightCore::Transport::HilFrameParser parser{};
        FlightCore::Transport::HilHeader header{};
        std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload{};
        std::int64_t last_report_ms = 0;
        std::int64_t last_transport_report_ms = 0;
        std::int64_t last_status_ms = -kStatusPeriodMs;
        FlightCore::InterFc::NodeState node_state = FlightCore::InterFc::NodeState::Healthy;
        FlightCore::InterFc::DetectionCode detection = FlightCore::InterFc::DetectionCode::None;
    };

    /* Runs one iteration of the FC2 supervision loop. */
    void run_once(const Fc2Boot& boot, Fc2Session& session)
    {
        service_inter_fc_link(boot.inter_fc_transport, session.health, session.node_state,
                             session.detection, session.last_report_ms);
        if (session.reset_requested) {
            reset_supervision(session.health);
        }
        session.node_state = evaluate_fc1_health(session.health, session.detection);
        publish_fc2_status(boot.inter_fc_transport, session.node_state, session.detection, session.last_status_ms);
        report_inter_fc_transport(boot.inter_fc_transport, session.last_transport_report_ms);
        update_status_led(session.node_state, session.detection);
        std::uint8_t byte = 0;
        if (uart_poll_in(boot.uart, &byte) == 0) {
            const bool complete = session.parser.processByte(byte, session.header, session.payload);
            if (complete) {
                process_sensor(boot.uart, session.health, session.header,
                               std::span<const std::uint8_t>{session.payload.data(),
                                                             session.header.payload_len});
            }
        }
        else {
            k_sleep(K_MSEC(1));
        }
    }
}

/* Zephyr FC2 entry point. SensorPackets act as the initial supervision heartbeat. */
export int run_fc2_firmware()
{
    const auto led_initialized = status_led.initialize();

    if (!led_initialized.has_value()) {
        printk("[LED] initialization failed: %d\n", led_initialized.error());
    }
    const Fc2Boot boot = boot_fc2_hardware();
    if (!boot.ready) {
        return -1;
    }
    printk("[BOOT] firmware=fc2_stm32 role=FC2 board=nucleo_l476rg hil_baud=115200 "
           "inter_fc=USART3/PB10/PB11/115200\n");

    Fc2Session session{};
    inter_fc.startup_state = 4;

    while (true) {
        run_once(boot, session);
    }
}

}
