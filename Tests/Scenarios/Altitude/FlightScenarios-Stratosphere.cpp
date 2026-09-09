/*
Filename: Tests/Scenarios/Altitude/FlightScenarios-Stratosphere.cpp
Description: Long-duration autonomous climb to the 20 km stratosphere target.

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

/* Runs the long-duration Heliblade-derived climb to the 20 km target. */
void stratosphere_climb(TestHarness&                  runner,
                        std::float64_t                hover_rpm,
                        const sim::PhysicsDispersion& dispersion)
{
    runner.begin_scenario("NOMINAL-017", "Stratosphere climb (approximately 9 h, z = 20 km)");
    runner.log_header();

    sim::control::ControllerConfig config{.hover_rpm = hover_rpm};
    sim::control::FlightController controller{config, dispersion};
    Aircraft aircraft{dispersion};
    const MissionRunTrace trace = run_mission(
        controller,
        aircraft,
        {.target = {.z = 20000.0},
         .duration = 32430.0,
         .axis = TrackingAxis::z_axis,
         .tolerance = 2.0,
         .verbose = runner.verbose()},
        dispersion);

    const AircraftState& final_state = aircraft.state();
    print_metrics_report("altitude", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units,
                          trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error,
                          trace.metrics.max_acceleration);
    runner.check(std::abs(final_state.z - 20000.0) <= 2.0,
                 "stratosphere target reached (20 km +/- 2 m)");
    runner.check(trace.metrics.time_within_tolerance >= 32300.0,
                 "climb duration remains close to nine hours");
    runner.check(final_state.z > 0.0, "scenario ends airborne without landing");
}

}
