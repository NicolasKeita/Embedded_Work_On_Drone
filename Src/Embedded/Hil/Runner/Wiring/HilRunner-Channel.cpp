/*
Filename: Src/Embedded/Hil/Runner/Wiring/HilRunner-Channel.cpp
Description: Byte-channel and FC-target wiring of the HIL runner context factory.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FlightController;
import HilClock;
import HilConfig;
import HilFcTarget;
import HilRunnerContext;
import LoopbackTransport;
import SerialTransport;
import SilFaultScenario;
import Transport;

namespace sim::hil {

using HilChannelResult = std::expected<std::unique_ptr<FlightCore::Transport::ITransport>, HilError>;

/* Opens the loopback byte channel used by the host emulator target. */
std::unique_ptr<FlightCore::Transport::ITransport> open_loopback_channel()
{
    return std::unique_ptr<FlightCore::Transport::ITransport>{new FlightCore::Sim::LoopbackTransport{}};
}

/* Opens the serial byte channel of a physical target. */
HilChannelResult open_serial_channel(const HilConfig& config)
{
    SerialTransport::OpenResult opened = SerialTransport::openPort(config.interface_name);

    if (!opened.has_value()) {
        return std::unexpected(HilError::SerialOpenFailed);
    }
    return std::unique_ptr<FlightCore::Transport::ITransport>{opened->release()};
}

/* Opens the loopback or serial byte channel selected by the configuration. */
HilChannelResult open_hil_channel(const HilConfig& config)
{
    if (config.interface_name == "loopback") {
        return open_loopback_channel();
    }
    return open_serial_channel(config);
}

/* Attaches the host emulator target to a loopback channel. */
void attach_host_target(HilRunContext& ctx, const HilConfig& config)
{
    ctx.fc_target = std::make_unique<HostFcTarget>(*ctx.channel, ctx.clock, config.target,
                                                   config.controller, config.dt_s, config.sensor_limits);
}

/* Attaches the host or remote FC target matching the selected channel. */
void attach_fc_target(HilRunContext& ctx, const HilConfig& config)
{
    if (config.interface_name == "loopback") {
        attach_host_target(ctx, config);
        return;
    }
    ctx.fc_target = std::make_unique<RemoteFcTarget>();
}

}
