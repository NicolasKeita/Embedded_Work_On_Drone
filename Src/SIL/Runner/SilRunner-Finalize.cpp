/*
Filename: Src/SIL/Runner/SilRunner-Finalize.cpp
Description: Result finalization: latencies, terminal mission semantics and aggregates.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import FlightController;
import SilTypes;

namespace sim::sil {

/*
Finalizes the result: latencies, terminal mission semantics (ABORTED on safety
abort, FAILED when the window is exhausted, COMPLETE on success), duration,
running means and communication statistics.
*/
void SILRunner::finalize(RunContext& ctx)
{
    SimulationResult& result = ctx.result;

    if (result.fault_injected_time >= 0.0 && result.detection_time >= 0.0) {
        result.detection_latency = result.detection_time - result.fault_injected_time;
    }
    if (result.detection_time >= 0.0 && result.safety_response_time >= 0.0) {
        result.response_latency = result.safety_response_time - result.detection_time;
    }
    if (ctx.mission_abort_recorded) {
        result.final_state = sim::control::MissionState::ABORTED;
    }
    else if (result.final_state != sim::control::MissionState::COMPLETE) {
        result.final_state = sim::control::MissionState::FAILED;
    }
    result.mission_success = result.final_state == sim::control::MissionState::COMPLETE;
    result.mission_duration_s = ctx.mission_end_time >= 0.0 ? ctx.mission_end_time : ctx.time;
    if (ctx.metric_samples > 0) {
        result.mean_position_error_m = ctx.position_error_sum / static_cast<double>(ctx.metric_samples);
        result.mean_altitude_error_m = ctx.altitude_error_sum / static_cast<double>(ctx.metric_samples);
    }
    result.comms = ctx.comms_stats;
}

}
