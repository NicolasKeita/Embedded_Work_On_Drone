/*
Filename: Tests/Scenarios/FlightScenarios-Axes.cpp
Description: Implementation of the autonomous axis scenarios G, H and I.

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
Scenario G: autonomous altitude loop from the ground up to 100 m; measures the
time to enter tolerance, the overshoot and the residual error.
*/
void autonomous_altitude(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario G : autonomie, altitude pure (z : 0 -> 100 m) ==="
              << std::endl;
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}};
    Aircraft aircraft;

    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 90.0, .axis = TrackingAxis::z_axis, .tolerance = 2.0});

    print_metrics_report("altitude", trace.metrics);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "G1 : altitude dans la tolerance (+/- 2 m)");
    runner.check(trace.metrics.overshoot_percent() < 10.0, "G2 : depassement contenu (< 10 % de l'ecart initial)");
    runner.check(trace.metrics.steady_state_error < 1.0, "G3 : erreur residuelle faible (< 1 m)");
    runner.check(controller.state() == sim::control::MissionState::STATION_KEEPING
                     || controller.state() == sim::control::MissionState::COMPLETE,
                 "G4 : mission au moins en tenue de station");
}

/*
Test 2: cascaded X position -> pitch -> servos. Phase 1: rendezvous with point
(20, 0, 100), phase 2: return of x to 0 with metrics tracking.
*/
void autonomous_position_x(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario H : autonomie, axe X en cascade (x : 20 -> 0 m) ==="
              << std::endl;
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}};
    Aircraft aircraft;

    std::cout << "-- Phase 1 : ralliement du point (20, 0, 100) --" << std::endl;
    run_mission(controller, aircraft, {.target = {.x = 20.0, .z = 100.0}, .duration = 180.0,
                 .axis = TrackingAxis::x_axis, .tolerance = 1.0, .stop_on_zone = true});

    std::cout << "-- Phase 2 : retour vers x = 0 --" << std::endl;
    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 120.0, .axis = TrackingAxis::x_axis, .tolerance = 0.5});

    print_metrics_report("axe X", trace.metrics);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "H1 : consigne x = 0 atteinte (+/- 0,5 m)");
    runner.check(trace.metrics.overshoot_percent() < 15.0, "H2 : depassement contenu (< 15 % de l'ecart initial)");
    runner.check(trace.metrics.steady_state_error < 0.5, "H3 : erreur residuelle faible (< 0,5 m)");
}

/*
Test 3: cascaded Y position -> roll -> servos. Phase 1: rendezvous with point
(0, -15, 100), phase 2: return of y to 0 with metrics tracking.
*/
void autonomous_position_y(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario I : autonomie, axe Y en cascade (y : -15 -> 0 m) ==="
              << std::endl;
    runner.log_header();

    FlightController controller{ControllerConfig{.hover_rpm = hover_rpm}};
    Aircraft aircraft;

    std::cout << "-- Phase 1 : ralliement du point (0, -15, 100) --" << std::endl;
    run_mission(controller, aircraft, {.target = {.y = -15.0, .z = 100.0}, .duration = 180.0,
                 .axis = TrackingAxis::y_axis, .tolerance = 1.0, .stop_on_zone = true});

    std::cout << "-- Phase 2 : retour vers y = 0 --" << std::endl;
    const MissionRunTrace trace = run_mission(controller, aircraft,
                    {.target = {.z = 100.0}, .duration = 120.0, .axis = TrackingAxis::y_axis, .tolerance = 0.5});

    print_metrics_report("axe Y", trace.metrics);
    runner.check(trace.metrics.time_within_tolerance > 0.0, "I1 : consigne y = 0 atteinte (+/- 0,5 m)");
    runner.check(trace.metrics.overshoot_percent() < 15.0, "I2 : depassement contenu (< 15 % de l'ecart initial)");
    runner.check(trace.metrics.steady_state_error < 0.5, "I3 : erreur residuelle faible (< 0,5 m)");
}

}
