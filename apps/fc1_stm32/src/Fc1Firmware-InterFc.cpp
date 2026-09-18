/*
Filename: apps/fc1_stm32/src/Fc1Firmware-InterFc.cpp
Description: FC1 inter-FC link service: sends FC1 heartbeats, receives FC2
acknowledgements and statuses, and reports link progress periodically.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

module Fc1Firmware;

import std;

import InterFcLink;

namespace fc1 {

namespace {
    /* Consumes pending inter-FC messages and refreshes the remote supervision state. */
    void poll_inter_fc_messages(FlightCore::InterFc::IInterFcTransport& transport,
                                std::int64_t                            now_ms) noexcept
    {
        while (true) {
            const auto received = transport.poll();
            if (!received.has_value() || !received->has_value()) {
                break;
            }
            if ((*received)->kind == FlightCore::InterFc::MessageKind::Acknowledgement) {
                inter_fc.acknowledged_sequence = (*received)->sequence;
                inter_fc.acknowledgement_count = inter_fc.acknowledgement_count + 1;
            }
            if ((*received)->kind == FlightCore::InterFc::MessageKind::Acknowledgement
                || (*received)->kind == FlightCore::InterFc::MessageKind::Status) {
                inter_fc.last_remote_status_ms = now_ms;
                inter_fc.remote_state = static_cast<std::uint8_t>((*received)->state);
                inter_fc.remote_detection = static_cast<std::uint8_t>((*received)->detection);
            }
        }
    }
}

void service_inter_fc_link(const Fc1Links& links,
                           bool            heartbeat_suppressed,
                           std::int64_t&   last_heartbeat_ms,
                           std::int64_t&   last_report_ms) noexcept
{
    const std::int64_t now_ms = k_uptime_get();

    if (!heartbeat_suppressed && now_ms - last_heartbeat_ms >= kHeartbeatPeriodMs) {
        const FlightCore::InterFc::Message heartbeat{
            .kind = FlightCore::InterFc::MessageKind::Heartbeat,
            .sequence = inter_fc.next_sequence,
            .state = FlightCore::InterFc::NodeState::Healthy,
        };
        if (links.inter_fc_transport->send(heartbeat).has_value()) {
            inter_fc.heartbeat_count = inter_fc.heartbeat_count + 1;
            inter_fc.next_sequence = static_cast<std::uint16_t>(inter_fc.next_sequence + 1);
        }
        last_heartbeat_ms = now_ms;
    }
    poll_inter_fc_messages(*links.inter_fc_transport, now_ms);
    if (now_ms - last_report_ms >= kLinkReportPeriodMs) {
        printk("[INTERFC] role=FC1 heartbeats=%u acknowledgements=%u last_ack=%u "
               "remote_state=%u suppressed=%u\n",
               static_cast<unsigned int>(inter_fc.heartbeat_count),
               static_cast<unsigned int>(inter_fc.acknowledgement_count),
               static_cast<unsigned int>(inter_fc.acknowledged_sequence),
               static_cast<unsigned int>(inter_fc.remote_state),
               static_cast<unsigned int>(inter_fc.heartbeat_suppressed));
        last_report_ms = now_ms;
    }
}

}
