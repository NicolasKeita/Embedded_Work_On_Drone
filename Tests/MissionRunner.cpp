/*
Filename: Tests/MissionRunner.cpp
Description: Implementation of the step-by-step autonomous mission run loop.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;
import MissionSupport;

using sim::control::FlightController;
using sim::control::MissionState;
using sim::control::TargetState;

namespace
{
    void print_state_row(const Aircraft& aircraft, double timeSeconds)
    {
        const AircraftState& s = aircraft.state();
        std::cout << std::fixed << std::setw(9) << std::setprecision(2) << timeSeconds;
        std::cout << "   " << std::setprecision(3) << std::setw(11) << s.x
                  << std::setw(11) << s.y
                  << std::setw(11) << s.z;
        std::cout << "   " << std::setw(11) << s.vx
                  << std::setw(11) << s.vy
                  << std::setw(11) << s.vz;
        std::cout << "   " << std::setprecision(2) << std::setw(11) << s.pitch * 180.0 / std::numbers::pi
                  << std::setw(11) << s.roll * 180.0 / std::numbers::pi;
        std::cout << "   " << std::setprecision(0) << std::setw(11) << s.actual_rpm
                  << std::defaultfloat << std::endl;
    }

    bool inside_zone(const TargetState& t, const AircraftState& s, double tol)
    {
        return std::abs(t.x - s.x) <= tol && std::abs(t.y - s.y) <= tol
            && std::abs(t.z - s.z) <= tol;
    }
}

namespace sim::test {

MissionRunTrace run_mission(FlightController&        ctrl,
                            Aircraft&                craft,
                            const MissionRunRequest& run)
{
    constexpr double kDt = 0.01;
    constexpr double kSteadyWindowSeconds = 10.0;
    constexpr int kLogIntervalSteps = 250;

    MissionRunTrace trace;
    const double targetValue = component_value(run.target, run.axis);
    const double initialValue = component_value(craft.state(), run.axis);
    const double direction = targetValue >= initialValue ? 1.0 : -1.0;
    const double steadyStartTime = std::max(0.0, run.duration - kSteadyWindowSeconds);

    trace.metrics.initial_gap = std::abs(targetValue - initialValue);
    trace.visited_states.push_back(ctrl.state());

    MissionState previousState = ctrl.state();
    double time = 0.0;
    double steadyErrorSum = 0.0;
    double steadySamples = 0.0;
    int stepIndex = 0;

    print_state_row(craft, time);

    while (time < run.duration) {
        craft.set_command(ctrl.update(run.target, craft.state(), kDt));
        craft.update(kDt);
        time += kDt;
        ++stepIndex;

        const double error = targetValue - component_value(craft.state(), run.axis);

        trace.metrics.overshoot_units =
            std::max(trace.metrics.overshoot_units, std::max(-direction * error, 0.0));
        if (trace.metrics.time_within_tolerance < 0.0 && std::abs(error) <= run.tolerance) {
            trace.metrics.time_within_tolerance = time;
        }
        if (time >= steadyStartTime) {
            steadyErrorSum += std::abs(error);
            steadySamples += 1.0;
        }
        if (ctrl.state() != previousState) {
            previousState = ctrl.state();
            trace.visited_states.push_back(previousState);
            std::cout << "  [MISSION] t = " << std::fixed << std::setprecision(1) << time
                      << " s -> " << mission_state_name(previousState) << std::endl;
        }
        if (stepIndex % kLogIntervalSteps == 0) {
            print_state_row(craft, time);
        }

        const bool airborne = ctrl.state() != MissionState::TAKEOFF
                           && ctrl.state() != MissionState::CLIMB;
        if (run.stop_on_zone && airborne
            && inside_zone(run.target, craft.state(), run.tolerance)) {
            break;
        }
    }

    if (stepIndex % kLogIntervalSteps != 0 || time == run.duration) {
        print_state_row(craft, time);
    }

    trace.metrics.final_error =
        std::abs(targetValue - component_value(craft.state(), run.axis));
    trace.metrics.steady_state_error =
        steadySamples > 0.0 ? steadyErrorSum / steadySamples : trace.metrics.final_error;

    return trace;
}

}
