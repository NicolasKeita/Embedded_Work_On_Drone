/*
Filename: Src/SIL/Runner/Events/Faults/SilRunnerEvents-FaultReasons.cpp
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

/*
Nominal response the supervised system is expected to produce when the fault is
injected: watchdog bound, safety mode and mission outcome. The stated bounds
are the nominal configuration limits enforced by the validation suite.
*/
std::string_view fault_expected_behavior(const FaultScenario& scenario)
{
    switch (scenario.fault_type) {
    case FaultType::None:
        return {};
    case FaultType::FC1Failure:
        return "Watchdog must raise FC1_HEARTBEAT_TIMEOUT within 300ms, SafetyManager must "
               "engage SAFE_MODE with a controlled descent and the mission must be aborted.";
    case FaultType::CommunicationLoss:
        return "Watchdog must raise COMMUNICATION_LOST within 300ms, SafetyManager must "
               "engage SAFE_MODE and the mission must be aborted.";
    case FaultType::CommunicationLossRate:
        return "Link supervision must absorb the configured packet loss rate and raise "
               "COMMUNICATION_LOST only if the heartbeat timeout is exceeded.";
    case FaultType::SensorFault:
        return "Health monitor must invalidate the altitude channel on the range check "
               "within 500ms, FC1 must hold the last valid altitude, SafetyManager must "
               "engage COMPENSATED with nominal thrust and the flag must clear once the "
               "telemetry validates again.";
    case FaultType::ActuatorDegradation:
        return "Health monitor must raise ACTUATOR_MISMATCH after a sustained 500ms "
               "mismatch, SafetyManager must engage COMPENSATED with a 1.7x thrust margin "
               "within 600ms and the mission must continue.";
    }
    return {};
}

}
