/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Steps.cpp
Description: Per-step error event recording of the HIL runner (actuator packet rejected
with the transport failure reason).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerEvents;

import std;

import HilClock;
import HilEvents;
import HilRunnerContext;
import HilTransport;

namespace sim::hil {

namespace {
    std::string_view receive_result_detail(ReceiveResult result) noexcept
    {
        switch (result) {
        case ReceiveResult::SequenceError:
            return "sequence number mismatch";
        case ReceiveResult::MessageTypeError:
            return "unexpected message id";
        case ReceiveResult::EchoMismatch:
            return "stale sim-timestamp echo";
        case ReceiveResult::InvalidPayload:
            return "invalid payload";
        case ReceiveResult::Timeout:
            return "actuator timeout";
        case ReceiveResult::SendFailed:
            return "sensor send failed";
        case ReceiveResult::Ok:
            return "ok";
        }
        return "unknown";
    }
}

void record_step_error(HilRunContext& ctx, ReceiveResult result)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "HIL_RUNNER",
                              .type = HilEventType::HilStepError,
                              .severity = HilEventSeverity::Warning,
                              .detail = receive_result_detail(result),
                              .reason = "actuator packet rejected"});
}

}