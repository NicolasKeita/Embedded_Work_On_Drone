/*
Filename: Tests/Mission/MissionRunner.cppm
Description: Step-by-step autonomous mission run loop producing tracking metrics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MissionRunner;

import std;

import Aircraft;
import FlightController;
import MissionSupport;

using sim::control::FlightController;

export namespace sim::test {

struct MissionRunRequest {
    sim::control::TargetState target;
    double duration = 0.0;
    TrackingAxis axis = TrackingAxis::z_axis;
    double tolerance = 0.0;
    bool stop_on_zone = false;
};

struct MissionRunTrace {
    std::vector<sim::control::MissionState> visited_states;
    MissionMetrics metrics;
};

/*
Advances an autonomous mission step by step: applies the controller, integrates
the physics, periodically logs the state, traces the state machine transitions
and accumulates tracking metrics on the requested axis. With stop_on_zone, the
loop stops as soon as the vehicle holds the target zone.
*/
MissionRunTrace run_mission(FlightController& ctrl, Aircraft& craft,
                            const MissionRunRequest& run);

}
