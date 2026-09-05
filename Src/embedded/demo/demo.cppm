/*
Filename: Src/embedded/demo/demo.cppm
Description: HIL step-closure demo (single lockstep step) tying together the PC
simulator mocks and the Flight Controller HAL: a simulated sensor sample built
from private ground truth is sent as a SensorPacket over the loopback transport,
decoded through ISensorInput, run through a placeholder control law, written
through IActuatorOutput and echoed back as an ActuatorPacket, with round-trip
timing. Honours hil_validation.md (truth/sensor separation 4.3, PC time-master
lockstep 4.4) and HIL-Proto v1.0 (hil_protocol.md).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module flight.demo;

import std;

import flight.hal.types;
import flight.transport;
import flight.transport.protocol;
import flight.transport.protocol.parser;
import flight.sim.loopback;
import flight.sim.clock;
import flight.sim.sensor;
import flight.sim.actuator;

namespace FlightCore::Demo
{

/*
    Shared state of one lockstep step: the loopback channel, the simulated PC
    clock, the FC-facing sensor/actuator mocks, the frame parser and a payload
    scratch buffer.
*/
struct DemoContext
{
    Sim::LoopbackTransport                           transport{};
    Sim::SimulatedClock                              clock{};
    Sim::SimulatedSensorInput                        sensor_input{};
    Sim::SimulatedActuatorOutput                     actuator_output{};
    Transport::HilFrameParser                        parser{};
    std::array<std::uint8_t, Transport::kMaxPayload> payload_buffer{};
};

/*
    Ground-truth aircraft state kept private to the PC simulator; only a
    measurement derived from it is handed to the Flight Controller
    (hil_validation.md 4.3).
*/
struct TruthState
{
    std::float32_t altitude_m;
    std::float32_t climb_rate_ms;
};

/* Ground-truth -> simulated sensor model (measurement only, small noise). */
[[nodiscard]] HAL::SensorData buildSensorSample(std::uint64_t timestamp_us, const TruthState& truth) noexcept;

/* Placeholder FlightControllerCore: simple altitude hold plus wing levelling. */
void computeFakeControl(const HAL::SensorData& sensor, HAL::ActuatorCommands& cmds) noexcept;

/*
    Drains the bytes available on the transport through the parser until one
    complete, CRC-valid frame is assembled, or the transport is empty.
*/
[[nodiscard]] bool receiveFrame(Transport::ITransport& transport, Transport::HilFrameParser& parser,
                                Transport::HilHeader& out_header, std::span<std::uint8_t> out_payload);

/*
    Simulator half of the lockstep: builds the sensor sample from ground truth,
    sends the SensorPacket and lets the Flight Controller decode and consume it
    through ISensorInput.
*/
[[nodiscard]] bool exchangeSensorSample(DemoContext& context, std::uint16_t sequence, const TruthState& truth,
                                        HAL::SensorData& out_tx_sensor, HAL::SensorData& out_fc_sensor);

/*
    Flight Controller half of the lockstep: stamps the command set, applies the
    placeholder control law, writes through IActuatorOutput and decodes the
    ActuatorPacket echoed by the transport.
*/
[[nodiscard]] bool exchangeActuatorCommands(DemoContext& context, std::uint16_t sequence,
                                            const HAL::SensorData& fc_sensor, HAL::ActuatorCommands& cmds,
                                            Transport::HilActuatorPayload& out_decoded_actuator);

/*
    Prints the observable state of the step for validation (hil_validation.md
    4.4): decoded sensor values, FC commands, echoed timestamp and measured RTT.
*/
void printReport(std::uint64_t sim_us, std::uint16_t sequence, const HAL::SensorData& tx_sensor,
                 const HAL::SensorData& fc_sensor, const HAL::ActuatorCommands& cmds,
                 const Transport::HilActuatorPayload& actuator, std::uint64_t pc_rx_us,
                 std::uint64_t rejected);
}

export namespace FlightCore::Demo
{

/*
    Runs one full HIL lockstep step and reports the observable state. Returns 0
    on success, 1 when any exchange stage fails.
*/
[[nodiscard]] int runLockstepStep();

}
