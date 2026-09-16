/*
Filename: Src/Embedded/Transport/Codec/HilProtocolCodec.cppm
Description: HIL-Proto v1.1 HAL<->wire conversion and frame encoding layer:
builds/decodes the packed SensorPacket/ActuatorPacket payloads from/to the HAL
data structures and serializes complete frames (Header + Payload + CRC-16).
Implementations live in the hil_protocol_codec-*.cpp translation units.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilProtocolCodec;

import std;

import HalTypes;
import HilProtocol;
import HilProtocolParser;

export namespace FlightCore::Transport
{

/* Builds a HilSensorPayload from the HAL sensor snapshot. */
[[nodiscard]] HilSensorPayload makeSensorPayload(const FlightCore::HAL::SensorData& sensor,
                                                  const HilControlSetpoint& setpoint) noexcept;

/* Decodes a complete sensor payload; rejects invalid setpoint setpoints and hold durations. */
[[nodiscard]] bool decodeSensorPayload(std::span<const std::uint8_t> bytes, HilSensorPayload& out) noexcept;

/* Maps a decoded HilSensorPayload back onto the HAL sensor snapshot. */
[[nodiscard]] FlightCore::HAL::SensorData toSensorData(const HilSensorPayload& payload) noexcept;

/*
    Serializes a SensorPacket frame (Header + Payload + CRC-16, little-endian)
    ready for ITransport::sendBytes. sequence_num wraps at 65535 per HilHeader.
*/
[[nodiscard]] std::array<std::uint8_t, kSensorFrameSize> encodeSensorFrame(const HilSensorPayload& payload,
                                                                           std::uint16_t seq) noexcept;

/*
    Builds a HilActuatorPayload from the HAL command set. cmds.timestamp_us is
    used as the FC local clock (fc_timestamp_us); echo_sim_timestamp_us must be
    the sim_timestamp_us that was received in the triggering SensorPacket (RTT).
*/
[[nodiscard]] HilActuatorPayload makeActuatorPayload(const FlightCore::HAL::ActuatorCommands& cmds,
                                                     std::uint64_t echo_sim_timestamp_us,
                                                     const ActuatorDiagnostics& diagnostics);

/* Decodes raw payload bytes into a HilActuatorPayload; false on short input. */
[[nodiscard]] bool decodeActuatorPayload(std::span<const std::uint8_t> bytes, HilActuatorPayload& out) noexcept;

/* Maps a decoded HilActuatorPayload back onto the HAL command set. */
[[nodiscard]] FlightCore::HAL::ActuatorCommands toActuatorCommands(const HilActuatorPayload& payload) noexcept;

/* Serializes an ActuatorPacket frame (Header + Payload + CRC-16, little-endian). */
[[nodiscard]] std::array<std::uint8_t, kActuatorFrameSize> encodeActuatorFrame(const HilActuatorPayload& payload,
                                                                               std::uint16_t seq) noexcept;

}
