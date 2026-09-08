/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Transitions.cpp
Description: Fault detection and safety/mission state transition event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilRunnerContext;
import SilTypes;

namespace sim::sil {

using sim::safety::HealthReport;
using sim::safety::detection_event_name;
using sim::safety::safety_action_for;
using sim::safety::safety_action_name;
using sim::safety::safety_mode_name;

/*
Records the fault detection and classification chain on first detection: the
detection event that fired, then its classification.
*/
void record_detection(RunContext& ctx, const HealthReport& report)
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

    SilEvent detected{.timestamp = detection,
                      .source = "FC2",
                      .type = SilEventType::FaultDetected,
                      .severity = EventSeverity::Info,
                      .detail = detection_event_name(ctx.result.first_detection_event)};
    ctx.trace.record(detected);
    SilEvent classified = detected;
    classified.type = SilEventType::FaultClassified;
    classified.reason = "detection event classification";
    ctx.trace.record(classified);
}

/*
Records safety mode transitions and the first safety response: the response
detail names the safety action (ENTER_SAFE_MODE / ENTER_COMPENSATED /
RESUME_NORMAL), never a failure mode.
*/
void record_safety_transitions(RunContext& ctx)
{
    const sim::safety::SafetyMode current = ctx.safety.mode();
    const std::float64_t          response_time = ctx.safety.response_time();

    if (current != ctx.previous_safety_mode) {
        SilEvent transition{.timestamp = ctx.time,
                            .source = "FC2",
                            .type = SilEventType::SafetyStateTransition,
                            .severity = EventSeverity::Info,
                            .previous_state = safety_mode_name(ctx.previous_safety_mode),
                            .new_state = safety_mode_name(current),
                            .reason = current == sim::safety::SafetyMode::NORMAL
                                          ? "health restored" : detection_event_name(ctx.result.first_detection_event)};
        ctx.trace.record(transition);
        ctx.previous_safety_mode = current;
    }
    if (response_time >= 0.0 && !ctx.safety_response_recorded) {
        ctx.safety_response_recorded = true;
        ctx.result.safety_response_time = response_time;
        SilEvent response{.timestamp = response_time,
                          .source = "FC2",
                          .type = SilEventType::SafetyResponse,
                          .severity = EventSeverity::Info,
                          .detail = safety_action_name(safety_action_for(current))};
        ctx.trace.record(response);
    }
}

/* Records a mission state transition with its previous/new states and reason. */
void record_mission_transition(RunContext& ctx, sim::control::MissionState current)
{
    if (current == ctx.previous_mission_state) {
        return;
    }

    SilEvent transition{.timestamp = ctx.time,
                        .source = "FC1",
                        .type = SilEventType::MissionStateTransition,
                        .severity = EventSeverity::Info,
                        .previous_state = sim::control::mission_state_name(ctx.previous_mission_state),
                        .new_state = sim::control::mission_state_name(current),
                        .reason = current == sim::control::MissionState::COMPLETE ? "station hold completed"
                                  : "controller progression"};
    ctx.trace.record(transition);
    ctx.previous_mission_state = current;
    if (current == sim::control::MissionState::COMPLETE && ctx.mission_end_time < 0.0) {
        ctx.mission_end_time = ctx.time;
    }
}

}
