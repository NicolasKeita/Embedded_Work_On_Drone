/*
Filename: Src/Safety/HealthMonitor-Host.cpp
Description: Host adapter connecting the simulated communications bus to FC2 supervision.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

import CommsBus;
import Telemetry;

namespace sim::safety {

/* Adapts the host-only simulated bus to the platform-independent supervision input. */
void HealthMonitor::update_comms_flags(std::float64_t current_time, const sim::sil::CommsBus& comms)
{
    const LinkSupervision supervision{
        .link_up = comms.link_up(),
        .last_heartbeat_time = comms.last_received_time(),
    };
    update_comms_flags(current_time, supervision);
}

/* Preserves the host API while routing evaluation through the shared FC2 core. */
HealthReport HealthMonitor::evaluate(std::float64_t current_time,
                                     const sim::sil::CommsBus& comms,
                                     const sim::sil::SensorTelemetry& telemetry,
                                     std::float64_t commanded_rpm)
{
    const LinkSupervision supervision{
        .link_up = comms.link_up(),
        .last_heartbeat_time = comms.last_received_time(),
    };
    return evaluate(current_time, supervision, telemetry, commanded_rpm);
}

}
