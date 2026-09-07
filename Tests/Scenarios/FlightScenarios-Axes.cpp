/*
Filename: Tests/Scenarios/FlightScenarios-Axes.cpp
Description: Implementation of the autonomous axis scenarios (NOMINAL-001, NOMINAL-008, NOMINAL-009).

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
NOMINAL-001: autonomous altitude loop from the ground up to 100 m;
measures the time to enter tolerance, the overshoot and the residual error.
*/
void autonomous_altitude(TestHarness& runner, std::float64_t hover_rpm,
                         const sim::PhysicsDispersion& dispersion)
{
    runner.begin_scenario("NOMINAL-001", "Autonomous altitude hold (z: 0 -> 100 m)");
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}, dispersion};
    Aircraft aircraft{dispersion};

    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 90.0, .axis = TrackingAxis::z_axis, .tolerance = 2.0},
                    dispersion);

    print_metrics_report("altitude", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units, trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error, trace.metrics.max_acceleration);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "altitude within tolerance (+/- 2 m)");
    runner.check(trace.metrics.overshoot_percent() < 10.0, "overshoot contained (< 10 % of initial gap)");
    runner.check(trace.metrics.steady_state_error < 1.0, "low residual error (< 1 m)");
    runner.check(controller.state() == sim::control::MissionState::STATION_KEEPING
                     || controller.state() == sim::control::MissionState::COMPLETE,
                 "mission at least in station keeping");
}

/*
NOMINAL-008: cascaded X position -> pitch -> servos. Phase 1:
rendezvous with point (20, 0, 100), phase 2: return of x to 0 with metrics tracking.
*/
void autonomous_position_x(TestHarness& runner, std::float64_t hover_rpm,
                           const sim::PhysicsDispersion& dispersion)
{
    runner.begin_scenario("NOMINAL-008", "Autonomous cascaded X axis (x: 20 -> 0 m)");
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}, dispersion};
    Aircraft aircraft{dispersion};

    std::cout << "-- Phase 1: rendezvous with point (20, 0, 100) --" << std::endl;
    run_mission(controller, aircraft, {.target = {.x = 20.0, .z = 100.0}, .duration = 180.0,
                 .axis = TrackingAxis::x_axis, .tolerance = 1.0, .stop_on_zone = true}, dispersion);

    std::cout << "-- Phase 2: return towards x = 0 --" << std::endl;
    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 120.0, .axis = TrackingAxis::x_axis, .tolerance = 0.5},
                    dispersion);

    print_metrics_report("X axis", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units, trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error, trace.metrics.max_acceleration);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "target x = 0 reached (+/- 0.5 m)");
    runner.check(trace.metrics.overshoot_percent() < 15.0, "overshoot contained (< 15 % of initial gap)");
    runner.check(trace.metrics.steady_state_error < 0.5, "low residual error (< 0.5 m)");
}

/*
NOMINAL-009: cascaded Y position -> roll -> servos. Phase 1:
rendezvous with point (0, -15, 100), phase 2: return of y to 0 with metrics tracking.
*/
void autonomous_position_y(TestHarness& runner, std::float64_t hover_rpm,
                           const sim::PhysicsDispersion& dispersion)
{
    runner.begin_scenario("NOMINAL-009", "Autonomous cascaded Y axis (y: -15 -> 0 m)");
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}, dispersion};
    Aircraft aircraft{dispersion};

    std::cout << "-- Phase 1: rendezvous with point (0, -15, 100) --" << std::endl;
    run_mission(controller, aircraft, {.target = {.y = -15.0, .z = 100.0}, .duration = 180.0,
                 .axis = TrackingAxis::y_axis, .tolerance = 1.0, .stop_on_zone = true}, dispersion);

    std::cout << "-- Phase 2: return towards y = 0 --" << std::endl;
    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 120.0, .axis = TrackingAxis::y_axis, .tolerance = 0.5},
                    dispersion);

    print_metrics_report("Y axis", trace.metrics);
    runner.record_metrics(trace.metrics.overshoot_units, trace.metrics.time_within_tolerance,
                          trace.metrics.steady_state_error, trace.metrics.max_acceleration);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "target y = 0 reached (+/- 0.5 m)");
    runner.check(trace.metrics.overshoot_percent() < 15.0, "overshoot contained (< 15 % of initial gap)");
    runner.check(trace.metrics.steady_state_error < 0.5, "low residual error (< 0.5 m)");
}

}
