/*
Filename: Src/SIL/Validation/FlightControlValidation-Classify.cpp
Description: Root-cause classification of one SIL outcome into a FailureReason.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import FlightController;
import SilTypes;

namespace sim::sil::validation {

/*
Root-cause classification of one SIL outcome. The first failing check wins so
that the failure reason stays a single canonical domain per run; the order
reflects the severity hierarchy (mission abort > undetected fault > heartbeat
supervision > safety mode > physical bounds > comms). Position and altitude
errors are also computed here from the SIL ground-truth fields so that the
collector can aggregate them in a single pass.
*/
SimulationResult MonteCarloRunner::classify(const Scenario&                   scenario,
                                            const sim::sil::SimulationResult& sil_result)
{
    SimulationResult     result{.sil_result = sil_result, .failure_reason = FailureReason::None};
    const std::float64_t dx = sil_result.final_x_m - 0.0;
    const std::float64_t dy = sil_result.final_y_m - 0.0;

    result.position_error_m = std::sqrt(dx * dx + dy * dy);
    result.altitude_error_m = std::abs(sil_result.final_altitude_m - scenario.target_altitude_m);

    if (sil_result.final_state == sim::control::MissionState::FAILED) {
        result.failure_reason = FailureReason::MissionFailed;
    } else if (sil_result.final_state == sim::control::MissionState::ABORTED) {
        result.failure_reason = FailureReason::MissionAborted;
    } else if (scenario.failure_mode != FailureMode::NONE && !sil_result.fault_detected) {
        result.failure_reason = FailureReason::FaultUndetected;
    } else if (scenario.failure_mode != FailureMode::NONE && !sil_result.supervision_triggered &&
               scenario.failure_mode != FailureMode::ACTUATOR_DEGRADED &&
               scenario.failure_mode != FailureMode::INVALID_SENSOR_DATA) {
        result.failure_reason = FailureReason::SupervisionMissed;
    } else if (scenario.failure_mode != FailureMode::NONE && !sil_result.safe_mode_reached &&
               scenario.failure_mode != FailureMode::ACTUATOR_DEGRADED) {
        result.failure_reason = FailureReason::SafetyModeNotReached;
    } else if (result.position_error_m > 50.0) {
        result.failure_reason = FailureReason::PositionExceeded;
    } else if (result.altitude_error_m > 25.0) {
        result.failure_reason = FailureReason::AltitudeExceeded;
    } else if (sil_result.comms.timeouts > 0 && scenario.failure_mode == FailureMode::FC_COMMUNICATION_LOSS) {
        result.failure_reason = FailureReason::CommsTimeout;
    }

    return result;
}

}
