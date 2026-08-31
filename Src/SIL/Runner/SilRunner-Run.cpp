/*
Filename: Src/SIL/Runner/SilRunner-Run.cpp
Description: Top-level simulation loop, metrics aggregation and result finalization.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import Aircraft;
import FlightController;
import SafetyManager;
import SilEvents;
import SilTypes;

namespace sim::sil {

using sim::safety::SafetyMode;

/*
Executes the per-step pipeline until the configured duration is reached.
*/
void SILRunner::execute(RunContext& ctx)
{
    const double dt = ctx.config.dt;

    record_run_start(ctx);
    for (ctx.time = 0.0; ctx.time <= ctx.config.duration_s + 0.5 * dt; ctx.time += dt) {
        apply_injectors(ctx);
        update_fc1(ctx);
        update_monitoring(ctx);
        apply_actuators(ctx);
        update_metrics(ctx);
    }
    finalize(ctx);
    record_run_end(ctx);
}

/*
Runs the given fault scenarios and returns the structured outcome.
*/
std::expected<SilRunOutput, SilError> SILRunner::run(std::span<const FaultScenario> scenarios)
{
    return make_context(config_, scenarios).and_then([](RunContext&& ctx) {
        execute(ctx);
        SilRunOutput output;

        output.result = ctx.result;
        output.events = ctx.trace.take_events();
        output.telemetry = std::move(ctx.telemetry_recorder.samples);
        return std::expected<SilRunOutput, SilError>{std::move(output)};
    });
}

/*
Metrics step: aircraft error aggregation, mission transition recording, abort
timestamping and fixed-rate telemetry sampling.
*/
void SILRunner::update_metrics(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;
    const AircraftState& state = ctx.aircraft.state();
    const double position_error = std::hypot(state.x - cfg.target.x, state.y - cfg.target.y);
    const double altitude_error = std::abs(state.z - cfg.target.z);

    ctx.result.max_position_error_m = std::max(ctx.result.max_position_error_m, position_error);
    ctx.result.max_altitude_error_m = std::max(ctx.result.max_altitude_error_m, altitude_error);
    ctx.result.max_pitch_rad = std::max(ctx.result.max_pitch_rad, std::abs(state.pitch));
    ctx.result.max_roll_rad = std::max(ctx.result.max_roll_rad, std::abs(state.roll));
    ctx.position_error_sum += position_error;
    ctx.altitude_error_sum += altitude_error;
    ++ctx.metric_samples;
    ctx.result.final_x_m = state.x;
    ctx.result.final_y_m = state.y;
    ctx.result.final_altitude_m = state.z;
    ctx.result.final_state = ctx.fc1.state();
    record_mission_transition(ctx, ctx.fc1.state());
    if (ctx.safety.mode() == SafetyMode::SAFE_MODE && !ctx.mission_abort_recorded) {
        ctx.mission_abort_recorded = true;
        if (ctx.mission_end_time < 0.0) {
            ctx.mission_end_time = ctx.time;
        }
    }
    ctx.telemetry_recorder.maybe_record(ctx.time, state, ctx.command, ctx.commanded_rpm);
}

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