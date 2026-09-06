/*
Filename: Src/Embedded/Hil/FcHost/FcHostApp.cppm
Description: Host FC emulator application : the stdio HIL-Proto frame loop driving the
REAL sim::control::FlightController core (the same core the future fc1_stm32 firmware
will run — not a HIL-specific controller). This is NOT the physical target; it exists
only to drive the HIL runner before the STM32 arrives and shares the FC algorithm with
the future embedded build.
Exports:
    run_fc1_host()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FcHostApp;

import std;

import Aircraft;
import FlightController;
import FlightControllerTypes;
import HilClock;
import HilProtocol;
import HilProtocolParser;
import Telemetry;

export namespace sim::hil {

/*
Runs the host FC emulator loop: reads HIL-Proto SensorPacket frames from standard input,
runs the Flight Controller core (validating and holding the last good sample on an
invalid sensor) and writes the answering ActuatorPacket frames to standard output.
*/
int run_fc1_host();

}

namespace sim::hil {

/*
Mutable state of the host FC emulator loop (module-internal): the Flight Controller
core, the mission constants, the wall clock, the frame parser and the held FC view.
*/
struct FcHostState {
    FcHostState(const sim::control::ControllerConfig& controller, const sim::control::TargetState& target,
                std::float64_t dt, const sim::sil::SensorValidationLimits& limits, IWallClock& clock)
        : fc{controller}, target{target}, dt{dt}, limits{limits}, clock{clock}
    {
    }

    sim::control::FlightController                               fc;
    sim::control::TargetState                                    target;
    std::float64_t                                               dt;
    sim::sil::SensorValidationLimits                             limits;
    IWallClock&                                                  clock;
    AircraftState                                                fc_view{};
    bool                                                         have_view = false;
    FlightCore::Transport::HilFrameParser                        parser{};
    std::array<std::uint8_t, FlightCore::Transport::kMaxPayload> payload_buffer{};
};

/* Handles one complete, CRC-valid frame: validates the SensorPacket and answers it. */
bool process_frame(FcHostState& state, const FlightCore::Transport::HilHeader& header);

}
