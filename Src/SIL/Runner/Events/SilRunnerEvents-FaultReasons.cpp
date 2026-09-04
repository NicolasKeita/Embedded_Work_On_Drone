/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-FaultReasons.cpp
Description: Translation of fault scenarios into human-readable reason labels.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import SilTypes;
import Telemetry;

namespace sim::sil {

/*
Human-readable sensor corruption label that also states whether the corruption
lasts until the end of the run (duration <= 0) or only inside its activation
window.
*/
static std::string_view sensor_corruption_reason(const FaultScenario& scenario)
{
    switch (scenario.parameters.corruption) {
    case SensorCorruptionMode::None:
        return {};
    case SensorCorruptionMode::AltitudeNaN:
        return scenario.duration <= 0.0 ? "ALTITUDE_NAN permanently" : "ALTITUDE_NAN temporarily";
    case SensorCorruptionMode::AltitudeOutOfRange:
        return scenario.duration <= 0.0 ? "ALTITUDE_OUT_OF_RANGE permanently"
                                        : "ALTITUDE_OUT_OF_RANGE temporarily";
    case SensorCorruptionMode::ExtremeNoise:
        return scenario.duration <= 0.0 ? "EXTREME_NOISE permanently" : "EXTREME_NOISE temporarily";
    }
    return {};
}

/*
Human-readable effect label appended to the injection marker. Every message
states both the injected effect and whether it is permanent (duration <= 0,
active until the end of the run) or temporary (bounded activation window).
*/
std::string_view fault_effect_reason(const FaultScenario& scenario)
{
    switch (scenario.fault_type) {
    case FaultType::None:
        return {};
    case FaultType::FC1Failure:
        return scenario.duration <= 0.0 ? "flight controller 1 heartbeat stopped permanently"
                                        : "flight controller 1 heartbeat stopped temporarily";
    case FaultType::CommunicationLoss:
        return scenario.duration <= 0.0 ? "FC1-FC2 communication link cut permanently"
                                        : "FC1-FC2 communication link cut temporarily";
    case FaultType::CommunicationLossRate:
        return scenario.duration <= 0.0 ? "random packet loss permanently"
                                        : "random packet loss temporarily";
    case FaultType::SensorFault:
        return sensor_corruption_reason(scenario);
    case FaultType::ActuatorDegradation:
        return scenario.duration <= 0.0 ? "actuator efficiency reduced permanently"
                                        : "actuator efficiency reduced temporarily";
    }
    return {};
}

}
