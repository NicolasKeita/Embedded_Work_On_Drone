/*
Filename: Tests/Scenarios/FlightScenarios-Reference.cpp
Description: Implementation of the no-fault reference run (NOMINAL-001): take off,
climb to 10 m and hold altitude for the 30-second mission with tracking metrics.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightScenarios;

import std;

import Aircraft;
import FlightController;
import MissionRunner;
import TestHarness;

namespace sim::test::flight_scenarios {

using sim::control::ControllerConfig;
using sim::control::FlightController;

/*
NOMINAL-001: the no-fault reference run. The drone takes off from the ground,
climbs to 10 m and holds that altitude for the full 30-second mission. The
tracking metrics (overshoot, time to enter tolerance, steady-state error and
peak acceleration) are recorded so the Monte-Carlo campaign can stress-test the
altitude loop under dispersed physics.
*/
void autonomous_altitude_hold(TestHarness&                  runner,
                              std::float64_t                hover_rpm,
                              const sim::PhysicsDispersion& dispersion)
{
    runner.begin_scenario("NOMINAL-001", "Autonomous altitude hold (z: 0 -> 10 m)");
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}, dispersion};
    Aircraft aircraft{dispersion};

    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 10.0}, .duration = 30.0, .axis = TrackingAxis::z_axis, .tolerance = 1.0,
                     .verbose = runner.verbose()},
                    dispersion);

    print_metrics_report("altitude", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units, trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error, trace.metrics.max_acceleration);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "altitude within tolerance (+/- 1 m)");
    runner.check(trace.metrics.overshoot_percent() < 10.0, "overshoot contained (< 10 % of initial gap)");
    runner.check(trace.metrics.steady_state_error < 1.0, "low residual error (< 1 m)");
    runner.check(controller.state() == sim::control::MissionState::STATION_KEEPING
                     || controller.state() == sim::control::MissionState::COMPLETE,
                 "mission at least in station keeping");
}

}
