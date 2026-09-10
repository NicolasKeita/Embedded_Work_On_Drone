/*
Filename: Tests/Mission/MissionRunner-Metrics.cpp
Description: StepMetrics member definitions of the mission simulation loop.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;

namespace sim::test {

/* Accumulates the overshoot, tolerance-entry time and steady-state error of a run. */
void StepMetrics::update(MissionMetrics& metrics, std::float64_t time, std::float64_t error,
                         std::float64_t tolerance)
{
    metrics.overshoot_units =
        std::max(metrics.overshoot_units, std::max(-direction * error, std::float64_t{0.0}));
    if (metrics.time_within_tolerance < 0.0 && std::abs(error) <= tolerance) {
        metrics.time_within_tolerance = time;
    }
    if (time >= steady_start) {
        error_sum += std::abs(error);
        samples += 1.0;
    }
}

/* Tracks the peak acceleration magnitude from the velocity delta of one step. */
void StepMetrics::track_acceleration(std::float64_t dt, const AircraftState& current)
{
    const std::float64_t ax = (current.vx - previous_state.vx) / dt;
    const std::float64_t ay = (current.vy - previous_state.vy) / dt;
    const std::float64_t az = (current.vz - previous_state.vz) / dt;

    max_acceleration = std::max(max_acceleration, std::hypot(ax, ay, az));
    previous_state = current;
}

/* Closes the tracking metrics with the final error and the peak acceleration. */
void StepMetrics::close(MissionMetrics& metrics, std::float64_t target_value, TrackingAxis axis,
                        const AircraftState& final_state)
{
    metrics.final_error = std::abs(target_value - component_value(final_state, axis));
    metrics.steady_state_error = steady_state_error(metrics.final_error);
    metrics.max_acceleration = max_acceleration;
}

/* Computes the mean steady-state error, falling back to the final error. */
std::float64_t StepMetrics::steady_state_error(std::float64_t fallback) const
{
    return samples > 0.0 ? error_sum / samples : fallback;
}

}