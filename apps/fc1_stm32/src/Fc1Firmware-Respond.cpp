/*
Filename: apps/fc1_stm32/src/Fc1Firmware-Respond.cpp
Description: FC1 outgoing messages: monitoring sample publication to the FC2 supervisor
and the actuator response built and sent on the HIL console UART.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#include <zephyr/kernel.h>

module Fc1Firmware;

import std;

import Aircraft;
import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import InterFcLink;

namespace fc1 {

/* Publishes the monitoring sample of one control cycle to the FC2 supervisor. */
void publish_monitoring_sample(const Fc1Links&                         links,
                               const FlightCore::Transport::HilHeader& header,
                               const FlightCore::HAL::SensorData&      sensor,
                               const ControlCommand&                   command) noexcept
{
    const FlightCore::InterFc::Message monitoring_sample{
        .kind = FlightCore::InterFc::MessageKind::MonitoringSample,
        .sequence = header.sequence_num,
        .state = FlightCore::InterFc::NodeState::Healthy,
        .detection = FlightCore::InterFc::DetectionCode::None,
        .altitude_m = sensor.altitude_baro_m,
        .actual_rpm = sensor.wing_rpm_meas,
        .commanded_rpm = static_cast<std::float32_t>(command.wing_rpm),
    };

    static_cast<void>(links.inter_fc_transport->send(monitoring_sample));
}

/* Builds the actuator packet of one control cycle from the FC1 command. */
FlightCore::HAL::ActuatorCommands build_actuator_packet(const ControlCommand& command,
                                                        std::uint8_t          mode_flags) noexcept
{
    return FlightCore::HAL::ActuatorCommands{
        .timestamp_us = static_cast<std::uint64_t>(k_uptime_get()) * 1000u,
        .wing_rpm_cmd = static_cast<std::float32_t>(command.wing_rpm),
        .left_servo_rad = static_cast<std::float32_t>(command.left_servo_angle * kRadiansPerDegree),
        .right_servo_rad = static_cast<std::float32_t>(command.right_servo_angle * kRadiansPerDegree),
        .aux_actuator_cmd = 0.0f,
        .mode_flags = mode_flags,
    };
}

/* Sends the actuator answer of one control cycle on the HIL console UART. */
void send_actuator_response(const Fc1Links&                          links,
                            const FlightCore::Transport::HilHeader&  header,
                            const FlightCore::HAL::ActuatorCommands& actuators,
                            std::uint64_t                            sim_timestamp_us) noexcept
{
    const FlightCore::Transport::ActuatorDiagnostics diagnostics{
        .fc_health_status = inter_fc.remote_state,
        .fc_detection_code = inter_fc.remote_detection,
    };
    const FlightCore::Transport::HilActuatorPayload response_payload =
        FlightCore::Transport::makeActuatorPayload(actuators, sim_timestamp_us, diagnostics);
    const std::array<std::uint8_t, FlightCore::Transport::kActuatorFrameSize> response =
        FlightCore::Transport::encodeActuatorFrame(response_payload, header.sequence_num);

    send_frame(links.uart, response);
    status_led.mark_activity(k_uptime_get());
}

}
