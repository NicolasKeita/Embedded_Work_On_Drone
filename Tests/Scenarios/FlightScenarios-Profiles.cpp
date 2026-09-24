/*
Filename: Tests/Scenarios/FlightScenarios-Profiles.cpp
Description: Five low-altitude 30-second takeoff profiles without landing. Each
profile is the simulation binding of one canonical scenario of the shared
FunctionalScenarios registry (identity, description, target, duration and
tracking tolerance).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightScenarios;

import std;

import Aircraft;
import FlightController;
import FunctionalScenarios;
import MissionRunner;
import SilRuntimeConfig;
import TestHarness;

namespace sim::test::flight_scenarios {
namespace {

/*
Executes one complete profile from rest and verifies that it remains airborne at
the end, reaches the requested low altitude and moves as requested. The mission
definition comes from the shared registry entry.
*/
void run_low_flight_profile(TestHarness&                  runner,
                            std::float64_t                hover_rpm,
                            const sim::PhysicsDispersion& dispersion,
                            std::string_view              id)
{
    const sim::test::FunctionalScenario& profile = *sim::test::find_functional_scenario(id);

    runner.begin_scenario(profile.id, profile.description);
    runner.log_header();

    sim::control::ControllerConfig config = sim::host::sil_controller_config(hover_rpm, profile.id);
    sim::control::FlightController controller{config, dispersion};
    Aircraft aircraft{dispersion};

    const MissionRunTrace trace = run_mission(
        controller,
        aircraft,
        {.target = profile.target,
         .duration = profile.duration_s,
         .axis = TrackingAxis::z_axis,
         .tolerance = profile.tracking_tolerance,
         .verbose = runner.verbose(),
         .report_period_s = sim::host::sil_report_period(profile.id),
         .wind_x_mps = profile.wind_x_mps,
         .wind_y_mps = profile.wind_y_mps,
         .wind_start_s = profile.wind_start_s,
         .wind_end_s = profile.wind_end_s,
         .wind_gust_period_s = profile.wind_gust_period_s},
        dispersion);

    const AircraftState& final_state = aircraft.state();
    print_metrics_report("altitude", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units,
                          trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error,
                          trace.metrics.max_acceleration);
    runner.check(final_state.z > 0.5, "aircraft remains airborne at the end of the run");
    runner.check(std::abs(final_state.z - profile.target.z) <= 1.0, "low-altitude target reached (+/- 1 m)");
    runner.check(std::abs(final_state.x - profile.target.x) <= 1.5, "longitudinal target reached (+/- 1.5 m)");
    runner.check(std::abs(final_state.y - profile.target.y) <= 1.5, "lateral target reached (+/- 1.5 m)");
    runner.check(trace.metrics.max_acceleration < 2.0, "takeoff acceleration remains gentle (< 2 m/s^2)");
}
}

/* Runs the 5 m vertical takeoff and hold profile. */
void low_vertical_takeoff(TestHarness&                  runner,
                          std::float64_t                hover_rpm,
                          const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "NOMINAL-012");
}

/* Runs the 6 m takeoff profile with a 4 m forward translation. */
void low_forward_takeoff(TestHarness&                  runner,
                         std::float64_t                hover_rpm,
                         const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "NOMINAL-013");
}

/* Runs the 7 m takeoff profile with a 4 m left translation. */
void low_lateral_takeoff(TestHarness&                  runner,
                         std::float64_t                hover_rpm,
                         const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "NOMINAL-014");
}

/* Runs the 8 m takeoff profile with a positive diagonal translation. */
void low_diagonal_takeoff(TestHarness&                  runner,
                          std::float64_t                hover_rpm,
                          const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "NOMINAL-015");
}

/* Runs the 9 m takeoff profile with an opposed diagonal translation. */
void low_offset_takeoff(TestHarness&                  runner,
                        std::float64_t                hover_rpm,
                        const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "NOMINAL-016");
}


/* Runs the canonical crosswind profile with the shared configured wind envelope. */
void steady_crosswind(TestHarness& runner, std::float64_t hover_rpm,
                      const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "WIND-001");
}

/* Runs the canonical diagonal gust profile with the shared configured wind envelope. */
void diagonal_gusts(TestHarness& runner, std::float64_t hover_rpm,
                     const sim::PhysicsDispersion& dispersion)
{
    run_low_flight_profile(runner, hover_rpm, dispersion, "WIND-002");
}

}
