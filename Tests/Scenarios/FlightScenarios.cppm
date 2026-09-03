/*
Filename: Tests/Scenarios/FlightScenarios.cppm
Description: Autonomous flight scenarios G to J : altitude, cascaded X/Y position loops and full mission.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightScenarios;

import std;

import TestHarness;

export namespace sim::test::flight_scenarios {

// Scenario G: autonomous altitude loop, convergence towards z = 100 m with metrics.
void autonomous_altitude(TestHarness& runner, std::float64_t hover_rpm);

// Scenario H: cascaded X position -> pitch -> servos, return from x = 20 m to x = 0.
void autonomous_position_x(TestHarness& runner, std::float64_t hover_rpm);

// Scenario I: cascaded Y position -> roll -> servos, return from y = -15 m to y = 0.
void autonomous_position_y(TestHarness& runner, std::float64_t hover_rpm);

// Scenario J : mission complete TAKEOFF -> CLIMB -> STATION_KEEPING -> COMPLETE.
void autonomous_mission(TestHarness& runner, std::float64_t hover_rpm);

}
