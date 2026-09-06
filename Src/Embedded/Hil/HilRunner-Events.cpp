/*
Filename: Src/Embedded/Hil/HilRunner-Events.cpp
Description: Structured event recording of the HIL runner : run lifecycle, mission and
safety transitions, fault injection/clearing, FC1 failure and heartbeat events.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FlightController;
import HealthMonitor;
import HilEvents;
import FaultInjectors;
import HilClock;
import HilRunnerContext;
import SafetyManager;
import SilFaultScenario;

namespace sim::hil {

void record_run_start(HilRunContext& ctx)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "HIL_RUNNER",
                              .type = HilEventType::HilRunStart,
                              .severity = HilEventSeverity::Info,
                              .reason = "HIL run started"});
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC1",
                              .type = HilEventType::HilRunStart,
                              .severity = HilEventSeverity::Info,
                              .reason = "FC1 host emulator online (NOT the physical target)"});
}

void record_run_end(HilRunContext& ctx)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "HIL_RUNNER",
                              .type = HilEventType::HilRunEnd,
                              .severity = HilEventSeverity::Info,
                              .detail = sim::control::mission_state_name(ctx.result.final_state),
                              .reason = ctx.result.mission_success ? "mission completed" : "mission not completed"});
}

void record_mission_transition(HilRunContext& ctx, sim::control::MissionState current)
{
    if (current == ctx.previous_mission_state) {
        return;
    }
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC1",
                              .type = HilEventType::MissionStateTransition,
                              .severity = HilEventSeverity::Info,
                              .previous_state = sim::control::mission_state_name(ctx.previous_mission_state),
                              .new_state = sim::control::mission_state_name(current),
                              .reason = current == sim::control::MissionState::COMPLETE ? "station hold completed"
                                                                                        : "controller progression"});
    ctx.previous_mission_state = current;
    if (current == sim::control::MissionState::COMPLETE && ctx.result.mission_end_time < 0.0) {
        ctx.result.mission_end_time = ctx.time;
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                   .wall_us = ctx.clock->nowUs(),
                                   .source = "FC1",
                                   .type = HilEventType::MissionComplete,
                                   .severity = HilEventSeverity::Info,
                                   .reason = "MISSION_COMPLETE"});
    }
}

void record_safety_transitions(HilRunContext& ctx)
{
    const sim::safety::SafetyMode current = ctx.safety.mode();
    if (current != ctx.previous_safety_mode) {
        const std::string_view reason = current == sim::safety::SafetyMode::NORMAL ? "health restored"
                                          : sim::safety::fault_domain_name(ctx.result.first_fault_domain);
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                   .wall_us = ctx.clock->nowUs(),
                                   .source = "FC2",
                                   .type = HilEventType::SafetyStateTransition,
                                   .severity = HilEventSeverity::Info,
                                   .previous_state = sim::safety::safety_mode_name(ctx.previous_safety_mode),
                                   .new_state = sim::safety::safety_mode_name(current),
                                   .reason = reason});
        ctx.previous_safety_mode = current;
    }
    const std::float64_t response_time = ctx.safety.response_time();
    if (response_time >= 0.0 && !ctx.safety_response_recorded) {
        ctx.safety_response_recorded = true;
        ctx.result.safety_response_time = response_time;
        ctx.trace.record(HilEvent{.sim_time_s = response_time,
                                   .wall_us = ctx.clock->nowUs(),
                                   .source = "FC2",
                                   .type = HilEventType::SafetyStateTransition,
                                   .severity = HilEventSeverity::Info,
                                   .detail = sim::safety::safety_mode_name(current),
                                   .reason = "safety response engaged"});
    }
}

void record_detection(HilRunContext& ctx, const sim::safety::HealthReport& report)
{
    const std::float64_t detection = report.first_detection_time();
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
    ctx.trace.record(HilEvent{.sim_time_s = detection,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC2",
                              .type = HilEventType::FaultDetected,
                              .severity = HilEventSeverity::Info,
                              .detail = sim::safety::fault_domain_name(ctx.result.first_fault_domain),
                              .reason = "fault detected"});
    if (ctx.result.first_fault_domain == sim::safety::FaultDomain::FC1Heartbeat) {
        ctx.trace.record(HilEvent{.sim_time_s = detection,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FC2",
                                  .type = HilEventType::HeartbeatTimeout,
                                  .severity = HilEventSeverity::Warning,
                                  .reason = "FC1 heartbeat / actuator-link timeout"});
    }
}

void record_fc1_failure_event(HilRunContext& ctx)
{
    ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                              .wall_us = ctx.clock->nowUs(),
                              .source = "FC1",
                              .type = HilEventType::Fc1Failure,
                              .severity = HilEventSeverity::Warning,
                              .detail = "FC1_FAILURE"});
}

void record_fault_events(HilRunContext& ctx)
{
    bool any_active = false;
    sim::sil::FaultType active_type = sim::sil::FaultType::None;
    for (std::size_t i = 0; i < ctx.injector_count; ++i) {
        if (ctx.injectors[i].is_active(ctx.time)) {
            any_active = true;
            active_type = ctx.injectors[i].scenario().fault_type;
            break;
        }
    }
    if (any_active && !ctx.fault_active) {
        ctx.fault_active = true;
        ctx.last_fault_type = active_type;
        if (!ctx.fault_recorded) {
            ctx.fault_recorded = true;
            ctx.result.fault_injected_time = ctx.time;
        }
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultInjected,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::fault_type_name(active_type),
                                  .reason = "fault injected into the HIL data path"});
        if (active_type == sim::sil::FaultType::FC1Failure && ctx.fc1_was_alive) {
            ctx.fc1_was_alive = false;
            record_fc1_failure_event(ctx);
        }
    }
    else if (!any_active && ctx.fault_active) {
        ctx.fault_active = false;
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FAULT_INJECTOR",
                                  .type = HilEventType::FaultCleared,
                                  .severity = HilEventSeverity::Info,
                                  .detail = sim::sil::fault_type_name(ctx.last_fault_type),
                                  .reason = "fault window ended"});
    }
}

}
