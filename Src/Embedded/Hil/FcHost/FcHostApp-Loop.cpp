/*
Filename: Src/Embedded/Hil/FcHost/FcHostApp-Loop.cpp
Description: stdio frame loop of the host FC emulator : reads SensorPacket frames from
standard input and writes the answering ActuatorPacket frames to standard output.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module;

#if defined(_WIN32)
#include <cstdio>
#include <fcntl.h>
#include <io.h>

module FcHostApp;

import std;

import HilClock;
import HilConfig;
import HilProtocol;
import HilProtocolParser;
import HilScenarios;

namespace sim::hil {

int run_fc1_host()
{
    const sim::hil::HilConfig base = sim::hil::hil_base_config();
    sim::hil::MonotonicClock  clock;

#if defined(_WIN32)
    _setmode(_fileno(stdin), _O_BINARY);
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    FcHostState state{base.controller, base.target, base.dt_s, base.sensor_limits, clock};

    std::array<std::uint8_t, FlightCore::Transport::kSensorFrameSize> frame{};
    for (;;) {
        const std::size_t got = std::fread(frame.data(), 1, frame.size(), stdin);
        if (got != frame.size()) {
            break;
        }
        FlightCore::Transport::HilHeader header{};
        for (std::uint8_t byte : frame) {
            if (state.parser.processByte(byte, header, state.payload_buffer) && !process_frame(state, header)) {
                break;
            }
        }
    }
    return 0;
}

}
