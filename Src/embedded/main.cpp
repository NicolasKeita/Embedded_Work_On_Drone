/*
Filename: Src/embedded/main.cpp
Description: HIL step-closure demo (single lockstep step). The PC simulator builds a
simulated sensor sample from its private ground-truth state, encodes a SensorPacket
over a LoopbackTransport; the Flight Controller decodes it, reads it through
ISensorInput, runs a placeholder control law, writes through IActuatorOutput and
emits an ActuatorPacket back over the transport; the host parses it and reports
round-trip timing. Honours hil_validation.md (truth/sensor separation, 4.3; PC
time-master lockstep, 4.4) and HIL-Proto v1.0 (hil_protocol.md).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import flight.hal.types;
import flight.hal.sensor;
import flight.hal.actuator;
import flight.hal.clock;
import flight.transport;
import flight.transport.protocol;
import flight.sim.sensor;
import flight.sim.actuator;
import flight.sim.clock;
import flight.sim.loopback;

namespace
{
using namespace FlightCore;

struct TruthState
{
    std::float32_t altitude_m;
    std::float32_t climb_rate_ms;
};

/*
    Ground-truth -> simulated sensor model. Only a measurement, with small noise,
    is exposed; the Flight Controller never receives TruthState directly
    (hil_validation.md 4.3).
*/
HAL::SensorData buildSensorSample(std::uint64_t timestamp_us, const TruthState& truth)
{
    HAL::SensorData sensor{};
    sensor.timestamp_us = timestamp_us;
    sensor.position_z_m = truth.altitude_m + 0.02f;
    sensor.altitude_baro_m = truth.altitude_m;
    sensor.velocity_z_ms = truth.climb_rate_ms;
    sensor.wing_rpm_meas = 1000.0f;
    sensor.roll_rad = 0.01f;
    sensor.pitch_rad = 0.02f;
    sensor.yaw_rad = 0.0f;
    sensor.sensor_valid_flags = HAL::kFlagImu1Ok | HAL::kFlagImu2Ok | HAL::kFlagBaroOk
                              | HAL::kFlagGpsFixOk | HAL::kFlagTachometerOk;
    return sensor;
}

/*
    Placeholder FlightControllerCore: simple altitude hold plus wing levelling
    (factice control law, not the production controller).
*/
void computeFakeControl(const HAL::SensorData& sensor, HAL::ActuatorCommands& cmds)
{
    constexpr std::float32_t target_altitude_m = 100.0f;
    constexpr std::float32_t hover_wing_rpm = 4000.0f;
    constexpr std::float32_t kp_altitude = 30.0f;
    const std::float32_t altitude_error = target_altitude_m - sensor.altitude_baro_m;
    cmds.wing_rpm_cmd = hover_wing_rpm + kp_altitude * altitude_error;
    cmds.left_servo_rad = -sensor.roll_rad;
    cmds.right_servo_rad = -sensor.roll_rad;
    cmds.aux_actuator_cmd = 0.0f;
    cmds.mode_flags = 2;
}

/*
    Drains the bytes available on the transport through the parser until one
    complete, CRC-valid frame is assembled, or the transport is empty.
*/
bool receiveFrame(Transport::ITransport& transport, Transport::HilFrameParser& parser,
                  Transport::HilHeader& out_header, std::span<std::uint8_t> out_payload)
{
    std::array<std::uint8_t, 128> rx{};
    while (transport.bytesAvailable() > 0)
    {
        const std::size_t n = transport.receiveBytes(rx);
        for (std::size_t i = 0; i < n; ++i)
        {
            if (parser.processByte(rx[i], out_header, out_payload)) return true;
        }
    }
    return false;
}

/*
    Prints the observable state of the step for validation (hil_validation.md 4.4):
    decoded sensor values, FC commands, echoed timestamp and measured RTT.
*/
void printReport(std::uint64_t sim_us, std::uint16_t sequence, const HAL::SensorData& tx_sensor,
                 const HAL::SensorData& fc_sensor, const HAL::ActuatorCommands& cmds,
                 const Transport::HilActuatorPayload& actuator, std::uint64_t pc_rx_us,
                 std::uint64_t rejected)
{
    const std::int64_t rtt_us = static_cast<std::int64_t>(pc_rx_us)
                              - static_cast<std::int64_t>(actuator.echo_sim_timestamp_us);
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "==== HIL lockstep step-closure ====" << "\n";
    std::cout << "sim_timestamp_us (PC master) : " << sim_us << "\n";
    std::cout << "sequence_num               : " << sequence << "\n";
    std::cout << "TX sensor alt_baro (m)     : " << tx_sensor.altitude_baro_m << "\n";
    std::cout << "RX decoded alt_baro (m)     : " << fc_sensor.altitude_baro_m << "\n";
    std::cout << "RX decoded wing_rpm_meas   : " << fc_sensor.wing_rpm_meas << "\n";
    std::cout << "FC wing_rpm_cmd             : " << cmds.wing_rpm_cmd << "\n";
    std::cout << "FC left_servo_rad           : " << cmds.left_servo_rad << "\n";
    std::cout << "FC right_servo_rad          : " << cmds.right_servo_rad << "\n";
    std::cout << "FC mode_flags              : " << static_cast<std::uint32_t>(cmds.mode_flags) << "\n";
    std::cout << "ActuatorPacket echo_sim_ts  : " << actuator.echo_sim_timestamp_us << "\n";
    std::cout << "ActuatorPacket fc_mode      : " << static_cast<std::uint32_t>(actuator.fc_mode) << "\n";
    std::cout << "ActuatorPacket health       : " << static_cast<std::uint32_t>(actuator.fc_health_status) << "\n";
    std::cout << "Round-trip time (us)        : " << rtt_us << "\n";
    std::cout << "Rejected frames by parser    : " << rejected << "\n";
}

}

using namespace FlightCore;

int main()
{
    Sim::LoopbackTransport transport;
    Sim::SimulatedClock clock;
    Sim::SimulatedSensorInput sensorInput;
    Sim::SimulatedActuatorOutput actuatorOutput;
    Transport::HilFrameParser parser;
    std::array<std::uint8_t, Transport::kMaxPayload> payload_buffer{};

    const std::uint16_t sequence = 1u;
    const std::uint64_t sim_step_us = 10000u;
    clock.advanceUs(sim_step_us);

    const TruthState truth{1.0f, 1.0f};
    const HAL::SensorData tx_sensor = buildSensorSample(clock.nowUs(), truth);

    const auto wire_sensor = Transport::makeSensorPayload(tx_sensor);
    const auto sensor_frame = Transport::encodeSensorFrame(wire_sensor, sequence);
    const bool sensor_frame_sent = transport.sendBytes(sensor_frame);
    if (!sensor_frame_sent) return 1;

    Transport::HilHeader header{};
    const bool sensor_ok = receiveFrame(transport, parser, header, payload_buffer);
    if (!sensor_ok) return 1;

    Transport::HilSensorPayload decoded_sensor{};
    const bool sensor_decoded = Transport::decodeSensorPayload(std::span<const std::uint8_t>(payload_buffer.data(), header.payload_len), decoded_sensor);
    if (!sensor_decoded) return 1;
    HAL::SensorData fc_sensor = Transport::toSensorData(decoded_sensor);
    sensorInput.inject(fc_sensor);
    const bool sensor_read = sensorInput.readSensorData(fc_sensor);
    if (!sensor_read) return 1;

    HAL::ActuatorCommands cmds{};
    cmds.timestamp_us = clock.nowUs();
    computeFakeControl(fc_sensor, cmds);
    const bool actuator_written = actuatorOutput.writeActuatorCommands(cmds);
    if (!actuator_written) return 1;

    const Transport::ActuatorDiagnostics diagnostics{};
    const auto wire_actuator = Transport::makeActuatorPayload(cmds, fc_sensor.timestamp_us, diagnostics);
    const auto actuator_frame = Transport::encodeActuatorFrame(wire_actuator, sequence);
    const bool actuator_frame_sent = transport.sendBytes(actuator_frame);
    if (!actuator_frame_sent) return 1;

    Transport::HilHeader actuator_header{};
    const bool actuator_ok = receiveFrame(transport, parser, actuator_header, payload_buffer);
    if (!actuator_ok) return 1;

    Transport::HilActuatorPayload decoded_actuator{};
    const bool actuator_decoded = Transport::decodeActuatorPayload(std::span<const std::uint8_t>(payload_buffer.data(), actuator_header.payload_len), decoded_actuator);
    if (!actuator_decoded) return 1;

    printReport(sim_step_us, sequence, tx_sensor, fc_sensor, cmds, decoded_actuator, clock.nowUs(), parser.rejectedFrames());
    return 0;
}
