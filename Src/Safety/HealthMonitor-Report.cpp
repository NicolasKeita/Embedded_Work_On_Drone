/*
Filename: Src/Safety/HealthMonitor-Report.cpp
Description: HealthReport query helpers (detection flags, first detection time and event).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HealthMonitor;

import std;

namespace sim::safety {

const DetectionFlag& HealthReport::flag(DetectionEvent event) const
{
    return flags[static_cast<std::size_t>(event)];
}

std::float64_t HealthReport::first_detection_time() const
{
    std::float64_t first = -1.0;

    for (const DetectionFlag& item : flags) {
        if (item.raised_time >= 0.0 && (first < 0.0 || item.raised_time < first)) {
            first = item.raised_time;
        }
    }
    return first;
}

DetectionEvent HealthReport::first_detection_event() const
{
    DetectionEvent best = DetectionEvent::FC1_HEARTBEAT_TIMEOUT;
    std::float64_t best_time = -1.0;

    for (std::size_t index = 0; index < flags.size(); ++index) {
        const DetectionFlag& item = flags[index];
        if (item.raised_time >= 0.0 && (best_time < 0.0 || item.raised_time < best_time)) {
            best_time = item.raised_time;
            best = static_cast<DetectionEvent>(index);
        }
    }
    return best;
}

}
