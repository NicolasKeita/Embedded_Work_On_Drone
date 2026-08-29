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
        double direction = 1.0;
        double steady_start = 0.0;
        double error_sum = 0.0;
        double samples = 0.0;

        void update(MissionMetrics& metrics, double time, double error, double tolerance)
        {
            metrics.overshoot_units = std::max(metrics.overshoot_units, std::max(-direction * error, 0.0));
            if (metrics.time_within_tolerance < 0.0 && std::abs(error) <= tolerance) {
                metrics.time_within_tolerance = time;
            }
            if (time >= steady_start) {
                error_sum += std::abs(error);
                samples += 1.0;
            }
        }

        [[nodiscard]] double steady_state_error(double fallback) const
        {
            return samples > 0.0 ? error_sum / samples : fallback;
        }
    };

    /* Records a state machine transition in the trace and reports it on the console. */
    void log_state_transition(MissionRunTrace& trace, MissionState& previous, MissionState current, double time)
    {
        if (current == previous) {
            return;
        }
        previous = current;
        trace.visited_states.push_back(current);
        std::cout << "  [MISSION] t = " << std::fixed << std::setprecision(1) << time
                  << " s -> " << mission_state_name(current) << std::endl;
    }

    // True when the run may stop: vehicle airborne and holding the target zone.
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
                      double                   direction,
                      double                   target_value)
{
    constexpr double kDt = 0.01;
    constexpr double kSteadyWindowSeconds = 10.0;
    constexpr int kLogIntervalSteps = 250;
    const double steady_start = std::max(0.0, run.duration - kSteadyWindowSeconds);
    StepMetrics step{direction, steady_start};
    MissionState previous_state = ctrl.state();
    double time = 0.0;
    int step_index = 0;

    print_state_row(craft, time);
    while (time < run.duration) {
        craft.set_command(ctrl.update(run.target, craft.state(), kDt));
        craft.update(kDt);
        time += kDt;
        ++step_index;
        const double error = target_value - component_value(craft.state(), run.axis);
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
}

}
