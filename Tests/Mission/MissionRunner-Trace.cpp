/*
Filename: Tests/Mission/MissionRunner-Trace.cpp
Description: State-transition tracing and target-zone detection of the mission loop.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MissionRunner;

import std;

import Aircraft;
import FlightController;

using sim::control::MissionState;

namespace sim::test {

/*
Records a state machine transition in the trace and reports it on the console.
*/
void log_state_transition(MissionRunTrace& trace,
                          MissionState&    previous,
                          MissionState     current,
                          std::float64_t   time,
                          bool             verbose)
{
    if (current == previous) {
        return;
    }
    previous = current;
    trace.record(current);
    if (verbose) {
        std::cout << "  [MISSION] t = " << std::fixed << std::setprecision(1) << time
                  << " s -> " << mission_state_name(current) << std::endl;
    }
}

/* Reports whether the vehicle holds the target zone and the run should stop there. */
bool zone_reached(const MissionRunRequest& run, MissionState state, const AircraftState& s)
{
    const bool airborne = state != MissionState::TAKEOFF && state != MissionState::CLIMB;
    const bool inside = std::abs(run.target.x - s.x) <= run.tolerance
        && std::abs(run.target.y - s.y) <= run.tolerance
        && std::abs(run.target.z - s.z) <= run.tolerance;

    return run.stop_on_zone && airborne && inside;
}

}
