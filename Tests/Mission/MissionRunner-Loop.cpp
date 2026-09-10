/*
Filename: Tests/Mission/MissionRunner-Loop.cpp
Description: Simulation loop internals of the step-by-step autonomous mission runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;

using sim::control::FlightController;
using sim::control::MissionState;

namespace sim::test {

namespace
{
    MissionViewerObserver mission_viewer_observer{};
    struct StepMetrics {
        std::float64_t direction = 1.0;
        std::float64_t steady_start = 0.0;
        std::float64_t error_sum = 0.0;
        std::float64_t samples = 0.0;
        std::float64_t max_acceleration = 0.0;
        AircraftState  previous_state{};

        void update(MissionMetrics& metrics, std::float64_t time, std::float64_t error, std::float64_t tolerance)
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
        void track_acceleration(std::float64_t dt, const AircraftState& current)
        {
            const std::float64_t ax = (current.vx - previous_state.vx) / dt;
            const std::float64_t ay = (current.vy - previous_state.vy) / dt;
            const std::float64_t az = (current.vz - previous_state.vz) / dt;

            max_acceleration = std::max(max_acceleration, std::hypot(ax, ay, az));
            previous_state = current;
        }

        /* Closes the tracking metrics with the final error and the peak acceleration. */
        void close(MissionMetrics&      metrics,
                   std::float64_t       target_value,
                   TrackingAxis         axis,
                   const AircraftState& final_state)
        {
            metrics.final_error = std::abs(target_value - component_value(final_state, axis));
            metrics.steady_state_error = steady_state_error(metrics.final_error);
            metrics.max_acceleration = max_acceleration;
        }

        [[nodiscard]] std::float64_t steady_state_error(std::float64_t fallback) const
        {
            return samples > 0.0 ? error_sum / samples : fallback;
        }
    };

    constexpr std::float64_t kDt = 0.01;
    constexpr std::float64_t kSteadyWindowSeconds = 10.0;
    constexpr std::uint32_t  kLogIntervalSteps = 250;
}

/* Registers the process-local observer used to visualize one accelerated mission. */
void set_mission_viewer_observer(MissionViewerObserver observer) noexcept
{
    mission_viewer_observer = observer;
}

/* Executes the step-by-step simulation loop, accumulating the tracking metrics. */
void run_control_loop(FlightController&        ctrl,
                      Aircraft&                craft,
                      const MissionRunRequest& run,
                      MissionRunTrace&         trace,
                      std::float64_t           direction,
                      std::float64_t           target_value)
{
    const std::float64_t     steady_start = std::max(std::float64_t{0.0}, run.duration - kSteadyWindowSeconds);
    StepMetrics              step{.direction = direction, .steady_start = steady_start,
                                  .previous_state = craft.state()};
    MissionState             previous_state = ctrl.state();
    std::float64_t           time = 0.0;
    std::uint32_t            step_index = 0;
    const std::float64_t     viewer_period = std::max(run.duration / std::float64_t{600.0},
                                                      std::float64_t{0.05});
    std::float64_t           next_viewer_time = 0.0;

    if (run.verbose) {
        print_state_row(craft, time);
    }
    while (time < run.duration) {
        craft.set_command(ctrl.update(run.target, craft.state(), kDt));
        craft.update(kDt);
        time += kDt;
        ++step_index;
        step.track_acceleration(kDt, craft.state());
        const std::float64_t error = target_value - component_value(craft.state(), run.axis);
        step.update(trace.metrics, time, error, run.tolerance);
        log_state_transition(trace, previous_state, ctrl.state(), time, run.verbose);
        if (mission_viewer_observer.callback != nullptr && time + std::float64_t{1.0e-9} >= next_viewer_time) {
            mission_viewer_observer.callback(
                MissionViewerSample{.time = time,
                                    .aircraft = craft.state(),
                                    .target = run.target,
                                    .mission = ctrl.state()},
                mission_viewer_observer.context);
            next_viewer_time += viewer_period;
        }
        if (run.verbose && step_index % kLogIntervalSteps == 0) {
            print_state_row(craft, time);
        }
        if (zone_reached(run, ctrl.state(), craft.state())) {
            break;
        }
    }
    if (run.verbose && (step_index % kLogIntervalSteps != 0 || time == run.duration)) {
        print_state_row(craft, time);
    }
    step.close(trace.metrics, target_value, run.axis, craft.state());
}

}
