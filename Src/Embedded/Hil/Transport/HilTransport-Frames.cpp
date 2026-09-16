/*
Filename: Src/Embedded/Hil/Transport/HilTransport-Frames.cpp
Description: Frame send/receive helpers of the HIL transport : SensorPacket and
ActuatorPacket encoding plus the non-blocking frame drain through the parser.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTransport;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolCodec;
import HilProtocolParser;
import Transport;

namespace sim::hil {

/* Sends sensor measurements and the scenario setpoint settings in one CRC-protected frame. */
bool send_sensor_frame(FlightCore::Transport::ITransport& channel,
                       const FlightCore::HAL::SensorData& sensor,
                       std::uint16_t                      sequence,
                       const FlightCore::Transport::HilControlSetpoint& setpoint) noexcept
{
    const auto payload = FlightCore::Transport::makeSensorPayload(sensor, setpoint);
    const auto frame = FlightCore::Transport::encodeSensorFrame(payload, sequence);

    return channel.sendBytes(frame);
}

bool send_actuator_frame(FlightCore::Transport::ITransport&                channel,
                         const FlightCore::HAL::ActuatorCommands&          cmds,
                         std::uint64_t                                     echo_sim_ts_us,
                         const FlightCore::Transport::ActuatorDiagnostics& diagnostics,
                         std::uint16_t                                     sequence) noexcept
{
    const auto payload = FlightCore::Transport::makeActuatorPayload(cmds, echo_sim_ts_us, diagnostics);
    const auto frame = FlightCore::Transport::encodeActuatorFrame(payload, sequence);

    return channel.sendBytes(frame);
}

bool receive_frame(FlightCore::Transport::ITransport&     channel,
                   FlightCore::Transport::HilFrameParser& parser,
                   FlightCore::Transport::HilHeader&      out_header,
                   std::span<std::uint8_t>                out_payload)
{
    std::array<std::uint8_t, 128> rx{};

    while (channel.bytesAvailable() > 0) {
        const std::size_t n = channel.receiveBytes(rx);
        for (std::size_t i = 0; i < n; ++i) {
            if (parser.processByte(rx[i], out_header, out_payload)) {
                return true;
            }
        }
    }
    return false;
}

}
