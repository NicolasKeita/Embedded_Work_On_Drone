/*
Filename: Src/SIL/Validation/Types/ValidationTypes.cpp
Description: Validation vocabulary : failure-reason names, ids and the scenario-to-fault projection.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ValidationTypes;

import std;

import SilTypes;

namespace sim::sil::validation {

/*
Canonical human-readable label of a failure reason. The labels are stable
literals so that CSV consumers and the report generator can reference them by
string_view without copying. None is exported as an empty view so that
successful runs show a blank reason column.
*/
std::string_view failure_reason_name(FailureReason reason)
{
    switch (reason) {
    case FailureReason::None:
        return {};
    case FailureReason::MissionAborted:
        return "MISSION_ABORTED";
    case FailureReason::MissionFailed:
        return "MISSION_FAILED";
    case FailureReason::FaultUndetected:
        return "FAULT_UNDETECTED";
    case FailureReason::WatchdogMissed:
        return "WATCHDOG_MISSED";
    case FailureReason::SafetyModeNotReached:
        return "SAFETY_MODE_NOT_REACHED";
    case FailureReason::PositionExceeded:
        return "POSITION_EXCEEDED";
    case FailureReason::AltitudeExceeded:
        return "ALTITUDE_EXCEEDED";
    case FailureReason::CommsTimeout:
        return "COMMS_TIMEOUT";
    case FailureReason::RunnerError:
        return "RUNNER_ERROR";
    }
    return {};
}

/*
Numeric identifier of a failure reason as a 32-bit value. It mirrors the enum
underlying value but is returned through a widening conversion so that the CSV
writer never has to cast again.
*/
std::uint32_t failure_reason_id(FailureReason reason)
{
    return static_cast<std::uint32_t>(reason);
}

/*
Projects the randomized scenario onto the declarative FaultScenario consumed by
the SIL engine. Only the fault-related fields are mapped: environment noise
and initial conditions are applied separately by the runner configuration.
*/
FaultScenario Scenario::to_fault_scenario() const
{
    return FaultScenario{.start_time = fault_start_time_s,
                         .duration = (fault_profile == FaultProfile::Permanent) ? 0.0 : fault_duration_s,
                         .fault_type = fault_type,
                         .target = fault_target,
                         .parameters = {.loss_probability = fault_loss_probability,
                                        .corruption = sensor_corruption,
                                        .corrupted_altitude_m = corrupted_altitude_m,
                                        .efficiency = actuator_efficiency}};
}

}
