/*
Filename: Tests/Scenarios/Altitude/FlightScenarios-Stratosphere.cpp
Description: Simulation binding of the long-duration autonomous climb to the 20 km
stratosphere target (NOMINAL-017). The mission definition comes from the shared
FunctionalScenarios registry.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightScenarios;

import std;

import Aircraft;
import FlightController;
import FunctionalScenarios;
import MissionRunner;
import TestHarness;

namespace sim::test::flight_scenarios {

/* Runs the long-duration Heliblade-derived climb to the 20 km target. */
void stratosphere_climb(TestHarness&                  runner,
                        std::float64_t                hover_rpm,
                        const sim::PhysicsDispersion& dispersion)
{
    const sim::test::FunctionalScenario& scenario = *sim::test::find_functional_scenario("NOMINAL-017");
    runner.begin_scenario(scenario.id, scenario.description);
    runner.log_header();

    sim::control::ControllerConfig config{.hover_rpm = hover_rpm};
    sim::control::FlightController controller{config, dispersion};
    Aircraft aircraft{dispersion};
    const MissionRunTrace trace = run_mission(
        controller,
        aircraft,
        {.target = scenario.target,
         .duration = scenario.duration_s,
         .axis = TrackingAxis::z_axis,
         .tolerance = scenario.tracking_tolerance,
         .verbose = runner.verbose()},
        dispersion);

    const AircraftState& final_state = aircraft.state();
    print_metrics_report("altitude", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units,
                          trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error,
                          trace.metrics.max_acceleration);
    runner.check(std::abs(final_state.z - scenario.target.z) <= scenario.tracking_tolerance,
                 "stratosphere target reached (20 km +/- 2 m)");
    runner.check(trace.metrics.time_within_tolerance >= 32300.0, "climb duration remains close to nine hours");
    runner.check(final_state.z > 0.0, "scenario ends airborne without landing");
}

}
