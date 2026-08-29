/*
Filename: Tests/Scenarios/Scenarios.cpp
Description: Implementation of the deterministic validation scenarios (A to F).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Scenarios;

import std;

import Aircraft;
import TestHarness;

namespace sim::test::scenarios {

void rest(TestHarness& runner, double)
{
    std::cout << "\n=== Scenario A : repos (RPM = 0, servos = 0) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    runner.run(aircraft, 3.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.z == 0.0, "A1 : l'appareil reste pose au sol (z = 0)");
    runner.check(s.vx == 0.0 && s.vy == 0.0 && s.vz == 0.0, "A2 : vitesses nulles");
    runner.check(s.pitch == 0.0 && s.roll == 0.0, "A3 : attitude neutre");
    runner.check(s.actual_rpm == 0.0, "A4 : RPM effectif nul");
}

void climb(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario B : montee (RPM = 1.1 x hover, servos = 0) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    aircraft.set_command({1.1 * hover_rpm, 0.0, 0.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.z > 1.0, "B1 : altitude en croissance (z > 1 m)");
    runner.check(s.vz > 0.0, "B2 : vitesse verticale positive");
    runner.check(s.actual_rpm > hover_rpm, "B3 : RPM effectif superieur au stationnaire");
}

void descent(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario C : descente (montee puis RPM = 0.6 x hover) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);
    const double topAltitude = aircraft.state().z;

    aircraft.set_command({0.6 * hover_rpm, 0.0, 0.0});
    runner.run(aircraft, 10.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.vz <= 0.0, "C1 : vitesse verticale negative en fin de phase");
    runner.check(s.z < topAltitude, "C2 : altitude inferieure au sommet atteint");
    runner.check(s.z == 0.0, "C3 : retour au sol (blocage a z = 0)");
}

void move_x(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario D : deplacement X (hover + pitch > 0) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({hover_rpm, 10.0, 10.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.pitch > 0.0, "D1 : tangage positif");
    runner.check(s.vx > 0.0, "D2 : vitesse X positive");
    runner.check(s.x > 1.0, "D3 : deplacement vers les X positifs (x > 1 m)");
    runner.check(s.y == 0.0 && s.vy == 0.0, "D4 : pas de derivation laterale");
}

void move_y(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario E : deplacement Y (hover + roll > 0) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({hover_rpm, 12.0, -12.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.roll > 0.0, "E1 : roulis positif");
    runner.check(s.vy > 0.0, "E2 : vitesse Y positive");
    runner.check(s.y > 1.0, "E3 : deplacement vers les Y positifs (y > 1 m)");
    runner.check(s.x == 0.0 && s.vx == 0.0, "E4 : pas de derivation longitudinale");
}

void combined(TestHarness& runner, double hover_rpm)
{
    std::cout << "\n=== Scenario F : combine (RPM > hover, pitch > 0, roll < 0) ===" << std::endl;
    runner.log_header();

    Aircraft aircraft;
    runner.take_off(aircraft, hover_rpm);

    aircraft.set_command({1.15 * hover_rpm, -5.0, 15.0});
    runner.run(aircraft, 6.0);

    const AircraftState& s = aircraft.state();
    runner.check(s.pitch > 0.0 && s.roll < 0.0, "F1 : attitude combinee (pitch > 0, roll < 0)");
    runner.check(s.vz > 0.0, "F2 : montee (vz > 0)");
    runner.check(s.vx > 0.0 && s.x > 1.0, "F3 : deplacement X positif");
    runner.check(s.vy < 0.0 && s.y < 0.0, "F4 : deplacement Y negatif");
}

}
