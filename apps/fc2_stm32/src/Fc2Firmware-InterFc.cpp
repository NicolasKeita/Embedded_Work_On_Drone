/*
Filename: apps/fc2_stm32/src/Fc2Firmware-InterFc.cpp
Description: FC2 inter-FC link service: receives FC1 heartbeats, acknowledges them,
refreshes the FC2 link supervision and reports link progress periodically.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

module Fc2Firmware;

import std;

import InterFcLink;

namespace fc2 {

namespace {
    /* Consumes pending FC1 messages and refreshes the supervised link state. */
    void poll_inter_fc_messages(FlightCore::InterFc::IInterFcTransport& transport,
                                Fc2Health& health,
                                FlightCore::InterFc::NodeState node_state,
                                FlightCore::InterFc::DetectionCode detection) noexcept
    {
        while (true) {
            const auto received = transport.poll();
            if (!received.has_value() || !received->has_value()) {
                break;
            }
            if ((*received)->kind == FlightCore::InterFc::MessageKind::ResetSupervision) {
                health.reset_requested = true;
                continue;
            }
            if ((*received)->kind == FlightCore::InterFc::MessageKind::MonitoringSample) {
                health.telemetry.z = (*received)->altitude_m;
                health.telemetry.actual_rpm = (*received)->actual_rpm;
                health.commanded_rpm = (*received)->commanded_rpm;
                continue;
            }
            if ((*received)->kind != FlightCore::InterFc::MessageKind::Heartbeat) {
                continue;
            }
            status_led.mark_activity(k_uptime_get());
            inter_fc.last_sequence = (*received)->sequence;
            inter_fc.heartbeat_count = inter_fc.heartbeat_count + 1;
            health.supervision.last_heartbeat_time = static_cast<std::float64_t>(k_uptime_get()) / 1000.0;
            const FlightCore::InterFc::Message acknowledgement{
                .kind = FlightCore::InterFc::MessageKind::Acknowledgement,
                .sequence = inter_fc.last_sequence,
                .state = node_state,
                .detection = detection,
            };
            if (transport.send(acknowledgement).has_value()) {
                inter_fc.acknowledgement_count = inter_fc.acknowledgement_count + 1;
            }
        }
    }
}

void service_inter_fc_link(FlightCore::InterFc::IInterFcTransport& transport,
                           Fc2Health& health,
                           FlightCore::InterFc::NodeState node_state,
                           FlightCore::InterFc::DetectionCode detection,
                           std::int64_t& last_report_ms) noexcept
{
    poll_inter_fc_messages(transport, health, node_state, detection);
    const std::int64_t now_ms = k_uptime_get();
    if (now_ms - last_report_ms >= kLinkReportPeriodMs) {
        printk("[INTERFC] role=FC2 heartbeats=%u acknowledgements=%u last_heartbeat=%u\n",
               static_cast<unsigned int>(inter_fc.heartbeat_count),
               static_cast<unsigned int>(inter_fc.acknowledgement_count),
               static_cast<unsigned int>(inter_fc.last_sequence));
        last_report_ms = now_ms;
    }
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

}
