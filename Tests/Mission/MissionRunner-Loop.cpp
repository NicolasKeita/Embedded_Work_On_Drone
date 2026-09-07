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
    struct StepMetrics {
        std::float64_t direction = 1.0;
        std::float64_t steady_start = 0.0;
        std::float64_t error_sum = 0.0;
        std::float64_t samples = 0.0;
        std::float64_t max_acceleration = 0.0;

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

        [[nodiscard]] std::float64_t steady_state_error(std::float64_t fallback) const
        {
            return samples > 0.0 ? error_sum / samples : fallback;
        }
    };

    /* Records a state machine transition in the trace and reports it on the console. */
    void log_state_transition(MissionRunTrace& trace, MissionState& previous, MissionState current, std::float64_t time)
    {
        if (current == previous) {
            return;
        }
        previous = current;
        trace.record(current);
        std::cout << "  [MISSION] t = " << std::fixed << std::setprecision(1) << time
                  << " s -> " << mission_state_name(current) << std::endl;
    }

    bool zone_reached(const MissionRunRequest& run, MissionState state, const AircraftState& s)
    {
        const bool airborne = state != MissionState::TAKEOFF && state != MissionState::CLIMB;
        const bool inside = std::abs(run.target.x - s.x) <= run.tolerance
            && std::abs(run.target.y - s.y) <= run.tolerance
            && std::abs(run.target.z - s.z) <= run.tolerance;

        return run.stop_on_zone && airborne && inside;
    }
}

/*
Executes the step-by-step simulation loop: applies the controller, integrates
the physics, periodically logs the state, traces the transitions and accumulates
the tracking metrics on the requested axis, then closes the metrics.
*/
void run_control_loop(FlightController&        ctrl,
                      Aircraft&                craft,
                      const MissionRunRequest& run,
                      MissionRunTrace&         trace,
                      std::float64_t           direction,
                      std::float64_t           target_value)
{
    constexpr std::float64_t kDt = 0.01;
    constexpr std::float64_t kSteadyWindowSeconds = 10.0;
    constexpr std::uint32_t  kLogIntervalSteps = 250;
    const std::float64_t     steady_start = std::max(std::float64_t{0.0}, run.duration - kSteadyWindowSeconds);
    StepMetrics              step{direction, steady_start};
    MissionState             previous_state = ctrl.state();
    std::float64_t           time = 0.0;
    std::uint32_t            step_index = 0;
    AircraftState             previous_aircraft_state = craft.state();

    print_state_row(craft, time);
    while (time < run.duration) {
        craft.set_command(ctrl.update(run.target, craft.state(), kDt));
        craft.update(kDt);
        time += kDt;
        ++step_index;
        const std::float64_t acceleration_x = (craft.state().vx - previous_aircraft_state.vx) / kDt;
        const std::float64_t acceleration_y = (craft.state().vy - previous_aircraft_state.vy) / kDt;
        const std::float64_t acceleration_z = (craft.state().vz - previous_aircraft_state.vz) / kDt;
        step.max_acceleration = std::max(step.max_acceleration,
            std::sqrt(acceleration_x * acceleration_x + acceleration_y * acceleration_y
                      + acceleration_z * acceleration_z));
        previous_aircraft_state = craft.state();
        const std::float64_t error = target_value - component_value(craft.state(), run.axis);
        step.update(trace.metrics, time, error, run.tolerance);
        log_state_transition(trace, previous_state, ctrl.state(), time);
        if (step_index % kLogIntervalSteps == 0) {
            print_state_row(craft, time);
        }
        if (zone_reached(run, ctrl.state(), craft.state())) {
            break;
        }
    }

    if (step_index % kLogIntervalSteps != 0 || time == run.duration) {
        print_state_row(craft, time);
    }
    trace.metrics.final_error = std::abs(target_value - component_value(craft.state(), run.axis));
    trace.metrics.steady_state_error = step.steady_state_error(trace.metrics.final_error);
    trace.metrics.max_acceleration = step.max_acceleration;
}

}
