/*
Filename: Tests/Mission/MissionRunner-Loop.cpp
Description: Viewer observer and the step-by-step simulation loop of the
autonomous mission runner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;
import SilRuntimeConfig;
import SilRunnerContext;

using sim::control::FlightController;
using sim::control::MissionState;

namespace sim::test {

MissionViewerObserver mission_viewer_observer{};

/* Registers the process-local observer used to visualize one accelerated mission. */
void set_mission_viewer_observer(MissionViewerObserver observer) noexcept
{
    mission_viewer_observer = observer;
}

/* Computes the simulation time after which the steady-state metrics window opens. */
std::float64_t steady_window_start(std::float64_t duration)
{
    return std::max(std::float64_t{0.0}, duration - sim::host::sil_runtime_options().steady_window_s);
}

/* Applies one control/physics step and accumulates the tracking metrics. */
void advance_step(MissionDynamics          dynamics,
                  StepMetrics&             step,
                  MissionRunTrace&         trace,
                  const MissionRunRequest& run,
                  std::float64_t&          time)
{
    dynamics.craft.set_command(dynamics.ctrl.update(run.target, dynamics.craft.state(), run.dt_s));
    const std::float64_t wind = sim::sil::wind_factor(
        run.wind_start_s, run.wind_end_s, run.wind_gust_period_s, time);
    dynamics.craft.set_wind(run.wind_x_mps * wind, run.wind_y_mps * wind);
    dynamics.craft.update(run.dt_s);
    time += run.dt_s;
    step.track_acceleration(run.dt_s, dynamics.craft.state());
    const std::float64_t error = component_value(dynamics.craft.state(), run.axis);
    const std::float64_t target = component_value(run.target, run.axis);
    step.update(trace.metrics, time, target - error, run.tolerance);
}

/* Publishes one compressed viewer sample at the visible replay cadence. */
void publish_viewer_sample(MissionDynamics          dynamics,
                           const MissionRunRequest& run,
                           std::float64_t           time,
                           std::float64_t&          next_viewer_time,
                           std::float64_t           viewer_period)
{
    const std::float64_t wind = sim::sil::wind_factor(
        run.wind_start_s, run.wind_end_s, run.wind_gust_period_s, time);
    mission_viewer_observer.callback(
        MissionViewerSample{.time = time,
                            .aircraft = dynamics.craft.state(),
                            .target = run.target,
                            .mission = dynamics.ctrl.state(),
                            .wind_x_mps = run.wind_x_mps * wind,
                            .wind_y_mps = run.wind_y_mps * wind,
                            .dt_s = run.dt_s},
        mission_viewer_observer.context);
    next_viewer_time += viewer_period;
}

/* Executes the step-by-step simulation loop, accumulating the tracking metrics. */
void run_control_loop(MissionDynamics          dynamics,
                      const MissionRunRequest& run,
                      MissionRunTrace&         trace,
                      std::float64_t           direction,
                      std::float64_t           target_value)
{
    StepMetrics          step{.direction = direction,
                              .steady_start = steady_window_start(run.duration),
                              .previous_state = dynamics.craft.state()};
    MissionState         previous_state = dynamics.ctrl.state();
    std::float64_t       time = 0.0;
    std::uint64_t        step_index = 0;
    const auto& options = sim::host::sil_runtime_options();
    const std::uint64_t log_interval_steps = std::max(std::uint64_t{1},
        static_cast<std::uint64_t>(run.report_period_s / run.dt_s + 0.5));
    const std::float64_t viewer_period = std::max(
        run.duration / static_cast<std::float64_t>(options.viewer_max_samples), options.viewer_period_s);
    std::float64_t       next_viewer_time = 0.0;

    if (run.verbose) {
        print_state_row(dynamics.craft, time);
    }
    while (time < run.duration) {
        advance_step(dynamics, step, trace, run, time);
        ++step_index;
        log_state_transition(trace, previous_state, dynamics.ctrl.state(), time, run.verbose);
        if (mission_viewer_observer.callback != nullptr && time + std::float64_t{1.0e-9} >= next_viewer_time) {
            publish_viewer_sample(dynamics, run, time, next_viewer_time, viewer_period);
        }
        if (run.verbose && step_index % log_interval_steps == 0) {
            print_state_row(dynamics.craft, time);
        }
        if (zone_reached(run, dynamics.ctrl.state(), dynamics.craft.state())) {
            break;
        }
    }
    if (run.verbose && (step_index % log_interval_steps != 0 || time == run.duration)) {
        print_state_row(dynamics.craft, time);
    }
    step.close(trace.metrics, target_value, run.axis, dynamics.craft.state());
}

}
