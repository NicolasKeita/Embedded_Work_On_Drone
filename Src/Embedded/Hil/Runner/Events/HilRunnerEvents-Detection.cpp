/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Detection.cpp
Description: Fault detection and FC1 failure event recording of the HIL runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerEvents;

import std;

import HealthMonitor;
import HilClock;
import HilEvents;
import HilRunnerContext;
import SilFaultScenario;

namespace sim::hil {

void record_detection(HilRunContext& ctx, const sim::safety::HealthReport& report)
{
    const std::float64_t detection = report.first_detection_time();

    if (detection < 0.0 || ctx.result.detection_time >= 0.0) {
        return;
    }
    ctx.result.detection_time = detection;
    ctx.result.first_detection_event = report.first_detection_event();
    ctx.result.fault_detected = true;
    if (ctx.detection_recorded) {
        return;
    }
    ctx.detection_recorded = true;
    ctx.trace.record(HilEvent{.sim_time_s = detection,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC2",
                              .type = HilEventType::FaultDetected,
                              .severity = HilEventSeverity::Info,
                              .detail = sim::safety::detection_event_name(ctx.result.first_detection_event),
                              .reason = "fault detected"});
    if (ctx.result.first_detection_event == sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT) {
        ctx.trace.record(HilEvent{.sim_time_s = detection,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FC2",
                                  .type = HilEventType::HeartbeatTimeout,
                                  .severity = HilEventSeverity::Warning,
                                  .reason = "FC1 heartbeat supervision timeout"});
    }
}

/*
Emits the FC1 failure event: the injection-side observable of the
FC1_UNAVAILABLE failure mode (the detail names the failure mode).
*/
void record_fc1_failure_event(HilRunContext& ctx)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC1",
                              .type = HilEventType::Fc1Failure,
                              .severity = HilEventSeverity::Warning,
                              .detail = sim::sil::failure_mode_name(sim::sil::FailureMode::FC1_UNAVAILABLE)});
}

}
