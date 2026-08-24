/*
Filename: Tests/Scenarios.cpp
Description: Implementation of the deterministic validation scenarios (A to F).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Scenarios;

import std;

import Aircraft;
import TestHarness;

void ScenarioRest(double)
{
    std::cout << "\n=== Scenario A : repos (RPM = 0, servos = 0) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    Run(aircraft, 0.0, 3.0);

    const AircraftState& s = aircraft.state();
    Check(s.z == 0.0, "A1 : l'appareil reste pose au sol (z = 0)");
    Check(s.vx == 0.0 && s.vy == 0.0 && s.vz == 0.0, "A2 : vitesses nulles");
    Check(s.pitch == 0.0 && s.roll == 0.0, "A3 : attitude neutre");
    Check(s.actual_rpm == 0.0, "A4 : RPM effectif nul");
}

void ScenarioClimb(double hoverRpm)
{
    std::cout << "\n=== Scenario B : montee (RPM = 1.1 x hover, servos = 0) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    aircraft.set_command({1.1 * hoverRpm, 0.0, 0.0});
    Run(aircraft, 0.0, 6.0);

    const AircraftState& s = aircraft.state();
    Check(s.z > 1.0, "B1 : altitude en croissance (z > 1 m)");
    Check(s.vz > 0.0, "B2 : vitesse verticale positive");
    Check(s.actual_rpm > hoverRpm, "B3 : RPM effectif superieur au stationnaire");
}

void ScenarioDescent(double hoverRpm)
{
    std::cout << "\n=== Scenario C : descente (montee puis RPM = 0.6 x hover) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    const double takeoffEnd = TakeOff(aircraft, hoverRpm);
    const double topAltitude = aircraft.state().z;

    aircraft.set_command({0.6 * hoverRpm, 0.0, 0.0});
    Run(aircraft, takeoffEnd, 10.0);

    const AircraftState& s = aircraft.state();
    Check(s.vz <= 0.0, "C1 : vitesse verticale negative en fin de phase");
    Check(s.z < topAltitude, "C2 : altitude inferieure au sommet atteint");
    Check(s.z == 0.0, "C3 : retour au sol (blocage a z = 0)");
}

void ScenarioMoveX(double hoverRpm)
{
    std::cout << "\n=== Scenario D : deplacement X (hover + pitch > 0) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    const double takeoffEnd = TakeOff(aircraft, hoverRpm);

    aircraft.set_command({hoverRpm, 10.0, 10.0});
    Run(aircraft, takeoffEnd, 6.0);

    const AircraftState& s = aircraft.state();
    Check(s.pitch > 0.0, "D1 : tangage positif");
    Check(s.vx > 0.0, "D2 : vitesse X positive");
    Check(s.x > 1.0, "D3 : deplacement vers les X positifs (x > 1 m)");
    Check(s.y == 0.0 && s.vy == 0.0, "D4 : pas de derivation laterale");
}

void ScenarioMoveY(double hoverRpm)
{
    std::cout << "\n=== Scenario E : deplacement Y (hover + roll > 0) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    const double takeoffEnd = TakeOff(aircraft, hoverRpm);

    aircraft.set_command({hoverRpm, 12.0, -12.0});
    Run(aircraft, takeoffEnd, 6.0);

    const AircraftState& s = aircraft.state();
    Check(s.roll > 0.0, "E1 : roulis positif");
    Check(s.vy > 0.0, "E2 : vitesse Y positive");
    Check(s.y > 1.0, "E3 : deplacement vers les Y positifs (y > 1 m)");
    Check(s.x == 0.0 && s.vx == 0.0, "E4 : pas de derivation longitudinale");
}

void ScenarioCombined(double hoverRpm)
{
    std::cout << "\n=== Scenario F : combine (RPM > hover, pitch > 0, roll < 0) ===" << std::endl;
    LogHeader();

    Aircraft aircraft;
    const double takeoffEnd = TakeOff(aircraft, hoverRpm);

    aircraft.set_command({1.15 * hoverRpm, -5.0, 15.0});
    Run(aircraft, takeoffEnd, 6.0);

    const AircraftState& s = aircraft.state();
    Check(s.pitch > 0.0 && s.roll < 0.0, "F1 : attitude combinee (pitch > 0, roll < 0)");
    Check(s.vz > 0.0, "F2 : montee (vz > 0)");
    Check(s.vx > 0.0 && s.x > 1.0, "F3 : deplacement X positif");
    Check(s.vy < 0.0 && s.y < 0.0, "F4 : deplacement Y negatif");
}