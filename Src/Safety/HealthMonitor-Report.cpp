/*
Filename: Src/Safety/HealthMonitor-Report.cpp
Description: HealthReport query helpers (flags, first detection time and domain).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

namespace sim::safety {

const FaultFlag& HealthReport::flag(FaultDomain domain) const
{
    return flags[static_cast<std::size_t>(domain)];
}

std::float64_t HealthReport::first_detection_time() const
{
    std::float64_t first = -1.0;

    for (const FaultFlag& item : flags) {
        if (item.raised_time >= 0.0 && (first < 0.0 || item.raised_time < first)) {
            first = item.raised_time;
        }
    }
    return first;
}

FaultDomain HealthReport::first_fault_domain() const
{
    FaultDomain    best = FaultDomain::FC1Heartbeat;
    std::float64_t best_time = -1.0;

    for (std::size_t index = 0; index < flags.size(); ++index) {
        const FaultFlag& item = flags[index];
        if (item.raised_time >= 0.0 && (best_time < 0.0 || item.raised_time < best_time)) {
            best_time = item.raised_time;
            best = static_cast<FaultDomain>(index);
        }
    }
    return best;
}

}
