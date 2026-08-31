/*
Filename: Src/SIL/Runner/Events/SilRunner-Transitions.cpp
Description: Fault detection and safety/mission state transition event recording.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTypes;

namespace sim::sil {

using sim::safety::HealthReport;
using sim::safety::fault_domain_name;
using sim::safety::safety_mode_name;

/* Builds a safety-mode command label for the safety response event. */
static std::string_view safety_command_name(sim::safety::SafetyMode mode)
{
    return mode == sim::safety::SafetyMode::SAFE_MODE      ? "ENTER_SAFE_MODE"
           : mode == sim::safety::SafetyMode::COMPENSATED  ? "ENTER_COMPENSATED"
                                                           : "RESUME_NORMAL";
}

/* Records the fault detection and classification chain on first detection. */
void SILRunner::record_detection(RunContext& ctx, const HealthReport& report)
{
    const double detection = report.first_detection_time();

    if (detection < 0.0 || ctx.result.detection_time >= 0.0) {
        return;
    }
    ctx.result.detection_time = detection;
    ctx.result.first_fault_domain = report.first_fault_domain();
    ctx.result.fault_detected = true;
    if (ctx.detection_recorded) {
        return;
    }
    ctx.detection_recorded = true;

    SilEvent detected;
    detected.timestamp = detection;
    detected.source = "FC2";
    detected.type = SilEventType::FaultDetected;
    detected.severity = EventSeverity::Info;
    detected.detail = fault_domain_name(ctx.result.first_fault_domain);
    ctx.trace.record(detected);
    SilEvent classified = detected;
    classified.type = SilEventType::FaultClassified;
    classified.reason = "fault domain classification";
    ctx.trace.record(classified);
}

/* Records safety mode transitions and the first safety response command. */
void SILRunner::record_safety_transitions(RunContext& ctx)
{
    const sim::safety::SafetyMode current = ctx.safety.mode();
    const double response_time = ctx.safety.response_time();

    if (current != ctx.previous_safety_mode) {
        SilEvent transition;
        transition.timestamp = ctx.time;
        transition.source = "FC2";
        transition.type = SilEventType::SafetyStateTransition;
        transition.severity = EventSeverity::Info;
        transition.previous_state = safety_mode_name(ctx.previous_safety_mode);
        transition.new_state = safety_mode_name(current);
        transition.reason = current == sim::safety::SafetyMode::NORMAL
            ? "health restored" : fault_domain_name(ctx.result.first_fault_domain);
        ctx.trace.record(transition);
        ctx.previous_safety_mode = current;
    }
    if (response_time >= 0.0 && !ctx.safety_response_recorded) {
        ctx.safety_response_recorded = true;
        ctx.result.safety_response_time = response_time;
        SilEvent response;
        response.timestamp = response_time;
        response.source = "FC2";
        response.type = SilEventType::SafetyResponse;
        response.severity = EventSeverity::Info;
        response.detail = safety_command_name(current);
        ctx.trace.record(response);
    }
}

/* Records a mission state transition with its previous/new states and reason. */
void SILRunner::record_mission_transition(RunContext& ctx, sim::control::MissionState current)
{
    if (current == ctx.previous_mission_state) {
        return;
    }

    SilEvent transition;
    transition.timestamp = ctx.time;
    transition.source = "FC1";
    transition.type = SilEventType::MissionStateTransition;
    transition.severity = EventSeverity::Info;
    transition.previous_state = sim::control::mission_state_name(ctx.previous_mission_state);
    transition.new_state = sim::control::mission_state_name(current);
    transition.reason = current == sim::control::MissionState::COMPLETE ? "station hold completed"
                                                        : "controller progression";
    ctx.trace.record(transition);
    ctx.previous_mission_state = current;
    if (current == sim::control::MissionState::COMPLETE && ctx.mission_end_time < 0.0) {
        ctx.mission_end_time = ctx.time;
    }
}

}