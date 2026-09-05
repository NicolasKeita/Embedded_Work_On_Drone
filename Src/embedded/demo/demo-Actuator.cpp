/*
Filename: Src/embedded/demo/demo-Actuator.cpp
Description: Flight-Controller-side actuator half of the HIL lockstep demo:
placeholder altitude-hold/levelling control law, command write through
IActuatorOutput and ActuatorPacket exchange over the loopback transport.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.demo;

import flight.hal.types;
import flight.transport;
import flight.transport.protocol;
import flight.transport.protocol.codec;
import flight.transport.protocol.parser;
import std;

namespace FlightCore::Demo
{

void computeFakeControl(const HAL::SensorData& sensor, HAL::ActuatorCommands& cmds) noexcept
{
    constexpr std::float32_t target_altitude_m = 100.0f;
    constexpr std::float32_t hover_wing_rpm = 4000.0f;
    constexpr std::float32_t kp_altitude = 30.0f;
    const std::float32_t     altitude_error = target_altitude_m - sensor.altitude_baro_m;

    cmds.wing_rpm_cmd = hover_wing_rpm + kp_altitude * altitude_error;
    cmds.left_servo_rad = -sensor.roll_rad;
    cmds.right_servo_rad = -sensor.roll_rad;
    cmds.aux_actuator_cmd = 0.0f;
    cmds.mode_flags = 2;
}

/*
    Flight Controller half of the lockstep: stamps the command set, applies the
    placeholder control law, writes through IActuatorOutput and decodes the
    ActuatorPacket echoed by the transport.
*/
bool exchangeActuatorCommands(DemoContext&                   context,
                              std::uint16_t                  sequence,
                              const HAL::SensorData&         fc_sensor,
                              HAL::ActuatorCommands&         cmds,
                              Transport::HilActuatorPayload& out_decoded_actuator)
{
    cmds.timestamp_us = context.clock.nowUs();
    computeFakeControl(fc_sensor, cmds);
    if (!context.actuator_output.writeActuatorCommands(cmds)) {
        return false;
    }

    const Transport::ActuatorDiagnostics diagnostics{};
    const auto wire_actuator = Transport::makeActuatorPayload(cmds, fc_sensor.timestamp_us, diagnostics);
    const auto actuator_frame = Transport::encodeActuatorFrame(wire_actuator, sequence);
    if (!context.transport.sendBytes(actuator_frame)) {
        return false;
    }

    Transport::HilHeader header{};
    if (!receiveFrame(context.transport, context.parser, header, context.payload_buffer)) {
        return false;
    }

    const std::span<const std::uint8_t> payload(context.payload_buffer.data(), header.payload_len);
    return Transport::decodeActuatorPayload(payload, out_decoded_actuator);
}

}
