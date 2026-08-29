/*
Filename: Tests/Mission/MissionRunner-Runner.cpp
Description: Entry point of the step-by-step autonomous mission run loop.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;

using sim::control::FlightController;

namespace sim::test {

/*
Runs the mission on the requested tracking axis and returns the accumulated
trace.
*/
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft, const MissionRunRequest& run)
{
    MissionRunTrace trace;
    const double target_value = component_value(run.target, run.axis);
    const double initial_value = component_value(craft.state(), run.axis);
    const double direction = target_value >= initial_value ? 1.0 : -1.0;

    trace.metrics.initial_gap = std::abs(target_value - initial_value);
    trace.visited_states.push_back(ctrl.state());

    run_control_loop(ctrl, craft, run, trace, direction, target_value);

    return trace;
}

}
