/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Finalize.cpp
Description: Result finalization of the HIL runner : detection/response latencies,
terminal mission semantics, comms/timing aggregation and verdict (safety-behaviour based,
with the Fail deadline policy overriding it).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FlightController;
import HilConfig;
import HilRunnerContext;
import HilTiming;
import HilTransport;

namespace sim::hil {

void HilRunner::finalize(HilRunContext& ctx)
{
    HilResult& result = ctx.result;

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

    result.comms = ctx.transport.stats();
    result.timing = ctx.timing;
    result.real_time_pacing = ctx.config.real_time_pacing;
    result.fault_expected = ctx.fault_expected;

    result.test_verdict = result.compute_verdict(ctx.fault_expected);
    if (ctx.config.deadline_policy == DeadlinePolicy::Fail && ctx.timing.deadline_misses > 0) {
        result.test_verdict = false;
    }
}

}
