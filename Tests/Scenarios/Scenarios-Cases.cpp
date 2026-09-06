/*
Filename: Tests/Scenarios/Scenarios-Cases.cpp
Description: Implementation of the deterministic validation scenarios (NOMINAL-002 to NOMINAL-007).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Scenarios;

import std;

import Aircraft;
import TestHarness;

namespace sim::test::scenarios {

void rest(TestHarness& runner, std::float64_t)
{
    runner.begin_scenario("NOMINAL-002", "Grounded rest (RPM = 0, servos = 0)");
    runner.log_header();

    Aircraft aircraft;
    runner.run(aircraft, 3.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.z == 0.0, "aircraft stays grounded (z = 0)");
    runner.check(s.vx == 0.0 && s.vy == 0.0 && s.vz == 0.0, "zero velocities");
    runner.check(s.pitch == 0.0 && s.roll == 0.0, "neutral attitude");
    runner.check(s.actual_rpm == 0.0, "zero effective RPM");
}

void climb(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-003", "Vertical climb (RPM = 1.1 x hover, servos = 0)");
    runner.log_header();

    Aircraft aircraft;
    aircraft.set_command({1.1 * hover_rpm, 0.0, 0.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.z > 1.0, "altitude increasing (z > 1 m)");
    runner.check(s.vz > 0.0, "positive vertical velocity");
    runner.check(s.actual_rpm > hover_rpm, "effective RPM above hover");
}

void descent(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-004", "Descent (climb then RPM = 0.6 x hover)");
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);
    const std::float64_t topAltitude = aircraft.state().z;

    aircraft.set_command({0.6 * hover_rpm, 0.0, 0.0});
    runner.run(aircraft, 10.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.vz <= 0.0, "negative vertical velocity at end of phase");
    runner.check(s.z < topAltitude, "altitude below reached peak");
    runner.check(s.z == 0.0, "return to ground (clamped at z = 0)");
}

void move_x(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-005", "Forward translation (hover + pitch > 0)");
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({hover_rpm, 10.0, 10.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.pitch > 0.0, "positive pitch");
    runner.check(s.vx > 0.0, "positive X velocity");
    runner.check(s.x > 1.0, "displacement towards positive X (x > 1 m)");
    runner.check(s.y == 0.0 && s.vy == 0.0, "no lateral drift");
}

void move_y(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-006", "Lateral translation (hover + roll > 0)");
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({hover_rpm, 12.0, -12.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.roll > 0.0, "positive roll");
    runner.check(s.vy > 0.0, "positive Y velocity");
    runner.check(s.y > 1.0, "displacement towards positive Y (y > 1 m)");
    runner.check(s.x == 0.0 && s.vx == 0.0, "no longitudinal drift");
}

void combined(TestHarness& runner, std::float64_t hover_rpm)
{
    runner.begin_scenario("NOMINAL-007", "Combined translation (RPM > hover, pitch > 0, roll < 0)");
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({1.15 * hover_rpm, -5.0, 15.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.pitch > 0.0 && s.roll < 0.0, "combined attitude (pitch > 0, roll < 0)");
    runner.check(s.vz > 0.0, "climb (vz > 0)");
    runner.check(s.vx > 0.0 && s.x > 1.0, "positive X displacement");
    runner.check(s.vy < 0.0 && s.y < 0.0, "negative Y displacement");
}

}
