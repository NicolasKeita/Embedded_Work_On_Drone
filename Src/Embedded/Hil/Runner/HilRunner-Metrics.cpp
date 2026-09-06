/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Metrics.cpp
Description: Per-step metric aggregation, mission progression, safety-abort recording and
fixed-rate dual telemetry sampling of truth and sensor streams.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import Aircraft;
import FlightController;
import HalTypes;
import HilClock;
import HilConfig;
import HilEvents;
import HilRunnerContext;
import HilRunnerEvents;
import HilTelemetry;
import SafetyManager;

namespace sim::hil {

namespace {
    /* Records the mission-abort event once the SafetyManager engages SAFE_MODE. */
    void record_mission_abort(HilRunContext& ctx)
    {
        if (ctx.safety.mode() != sim::safety::SafetyMode::SAFE_MODE || ctx.mission_abort_recorded) {
            return;
        }
        ctx.mission_abort_recorded = true;
        if (ctx.result.mission_end_time < 0.0) {
            ctx.result.mission_end_time = ctx.time;
        }
        ctx.trace.record(HilEvent{.sim_time_s = ctx.time,
                                  .wall_us = ctx.clock->nowUs(),
                                  .source = "FC2",
                                  .type = HilEventType::MissionAborted,
                                  .severity = HilEventSeverity::Warning,
                                  .reason = "mission aborted by SafetyManager"});
    }
}

void update_metrics(HilRunContext& ctx)
{
    const HilConfig&     cfg = ctx.config;
    const AircraftState& state = ctx.aircraft.state();
    const std::float64_t position_error = std::hypot(state.x - cfg.target.x, state.y - cfg.target.y);
    const std::float64_t altitude_error = std::abs(state.z - cfg.target.z);

    ctx.result.max_position_error_m = std::max(ctx.result.max_position_error_m, position_error);
    ctx.result.max_altitude_error_m = std::max(ctx.result.max_altitude_error_m, altitude_error);
    ctx.result.max_pitch_rad = std::max(ctx.result.max_pitch_rad, std::abs(state.pitch));
    ctx.result.max_roll_rad = std::max(ctx.result.max_roll_rad, std::abs(state.roll));
    ctx.result.final_x_m = state.x;
    ctx.result.final_y_m = state.y;
    ctx.result.final_altitude_m = state.z;

    sim::control::MissionState current = ctx.previous_mission_state;
    if (ctx.this_received) {
        current = static_cast<sim::control::MissionState>(ctx.this_fc.mission_state);
    }
    record_mission_transition(ctx, current);
    ctx.result.final_state = current;

    record_mission_abort(ctx);

    const HilTelemetryControl control{
        .target_x = cfg.target.x,
        .target_y = cfg.target.y,
        .target_z = cfg.target.z,
        .mission_state = static_cast<std::uint8_t>(current),
        .safety_state = static_cast<std::uint8_t>(ctx.safety.mode()),
    };
    ctx.telemetry_recorder.maybe_record(ctx.time, ctx.sampled_truth, ctx.last_sensor_data, ctx.actuator_cmd,
                                         ctx.commanded_rpm, control);
}

}
