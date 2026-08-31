/*
Filename: Src/SIL/Core/Events/SilEvents-Recorder.cpp
Description: Trace filtering/recording and telemetry sampling implementations.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilEvents;

import std;

import Aircraft;

namespace sim::sil {

SilTrace::SilTrace(SilTraceConfig config) : config_{config} {}

/*
Records one event when its severity belongs to the configured verbosity level.
*/
void SilTrace::record(SilEvent event)
{
    if (events_.size() >= config_.max_events) {
        return;
    }
    if (event_log_level(event.severity) > config_.level) {
        return;
    }
    events_.push_back(std::move(event));
}

std::span<const SilEvent> SilTrace::events() const noexcept
{
    return events_;
}

std::vector<SilEvent> SilTrace::take_events() noexcept
{
    return std::move(events_);
}

/*
Samples the aircraft state at the configured interval (simulation time based,
deterministic and independent from the internal integration tick).
*/
void TelemetryRecorder::maybe_record(double time, const AircraftState& state,
                                     const ControlCommand& command, double commanded_rpm)
{
    if (interval_s <= 0.0) {
        return;
    }
    if (time - last_sample_time + 1.0e-9 < interval_s) {
        return;
    }
    last_sample_time = time;

    TelemetrySample sample;
    sample.time = time;
    sample.x = state.x;
    sample.y = state.y;
    sample.altitude_m = state.z;
    sample.pitch_rad = state.pitch;
    sample.roll_rad = state.roll;
    sample.vx = state.vx;
    sample.vy = state.vy;
    sample.vz = state.vz;
    sample.commanded_rpm = commanded_rpm;
    sample.actual_rpm = state.actual_rpm;
    sample.left_servo_deg = command.left_servo_angle;
    sample.right_servo_deg = command.right_servo_angle;
    samples.push_back(sample);
}

}