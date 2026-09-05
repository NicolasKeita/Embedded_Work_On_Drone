/*
Filename: Src/embedded/demo/demo-Step.cpp
Description: Transport draining helper and single-lockstep-step orchestration for
the HIL step-closure demo (flight.demo).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module flight.demo;

import flight.hal.types;
import flight.transport;
import flight.transport.protocol;
import flight.transport.protocol.parser;
import std;

namespace FlightCore::Demo
{

/*
    Drains the bytes available on the transport through the parser until one
    complete, CRC-valid frame is assembled, or the transport is empty.
*/
bool receiveFrame(Transport::ITransport&     transport,
                  Transport::HilFrameParser& parser,
                  Transport::HilHeader&      out_header,
                  std::span<std::uint8_t>    out_payload)
{
    std::array<std::uint8_t, 128> rx{};

    while (transport.bytesAvailable() > 0) {
        const std::size_t n = transport.receiveBytes(rx);
        for (std::size_t i = 0; i < n; ++i) {
            if (parser.processByte(rx[i], out_header, out_payload)) {
                return true;
            }
        }
    }
    return false;
}

/*
    Runs one full HIL lockstep step: the PC simulator sends a SensorPacket built
    from its ground-truth state, the Flight Controller reacts and echoes an
    ActuatorPacket, and the observable state is reported (hil_validation.md 4.4).
*/
int runLockstepStep()
{
    DemoContext             context{};
    constexpr std::uint16_t sequence = 1u;
    constexpr std::uint64_t sim_step_us = 10000u;

    context.clock.advanceUs(sim_step_us);

    const TruthState truth{1.0f, 1.0f};

    HAL::SensorData tx_sensor{};
    HAL::SensorData fc_sensor{};
    if (!exchangeSensorSample(context, sequence, truth, tx_sensor, fc_sensor)) {
        return 1;
    }

    HAL::ActuatorCommands cmds{};
    Transport::HilActuatorPayload decoded_actuator{};
    if (!exchangeActuatorCommands(context, sequence, fc_sensor, cmds, decoded_actuator)) {
        return 1;
    }

    printReport(sim_step_us, sequence, tx_sensor, fc_sensor, cmds, decoded_actuator,
                context.clock.nowUs(), context.parser.rejectedFrames());
    return 0;
}

}
