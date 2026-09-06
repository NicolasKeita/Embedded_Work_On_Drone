/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Lifecycle.cpp
Description: Run lifecycle, mission transition and safety transition event recording of
the HIL runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerEvents;

import std;

import FlightController;
import HealthMonitor;
import HilClock;
import HilEvents;
import HilRunnerContext;
import SafetyManager;

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

}
