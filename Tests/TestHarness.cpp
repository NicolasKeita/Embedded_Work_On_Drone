/*
Filename: Tests/TestHarness.cpp
Description: Implementation of the shared validation harness (checks, logging, run loop).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module TestHarness;

import std;

namespace
{
    int g_failureCount = 0;
}

void Check(bool condition, const std::string& label)
{
    if (condition) {
        std::cout << "  [PASS] " << label << std::endl;
    }
    else {
        ++g_failureCount;
        std::cout << "  [FAIL] " << label << std::endl;
    }
}

int FailureCount()
{
    return g_failureCount;
}

void LogHeader()
{
    std::cout << "      t(s)";
    std::cout << std::setw(11) << "x(m)" << std::setw(11) << "y(m)"
              << std::setw(11) << "z(m)" << std::setw(11) << "vx(m/s)"
              << std::setw(11) << "vy(m/s)" << std::setw(11) << "vz(m/s)"
              << std::setw(11) << "pitch(d)" << std::setw(11) << "roll(d)"
              << std::setw(11) << "rpm" << std::endl;
}

void LogStep(double timeSeconds, const Aircraft& aircraft)
{
    const AircraftState& s = aircraft.state();

    std::cout << std::fixed << std::setw(9) << std::setprecision(2) << timeSeconds
              << std::setw(11) << std::setprecision(3) << s.x
              << std::setw(11) << s.y
              << std::setw(11) << s.z
              << std::setw(11) << s.vx
              << std::setw(11) << s.vy
              << std::setw(11) << s.vz
              << std::setw(11) << std::setprecision(2) << s.pitch * 180.0 / kPi
              << std::setw(11) << s.roll * 180.0 / kPi
              << std::setw(11) << std::setprecision(0) << s.actual_rpm
              << std::defaultfloat << std::endl;
}

// Avance la simulation de durationSecondes en loggant periodiquement l'etat.
void Run(Aircraft& aircraft, double startTimeSeconds, double durationSeconds)
{
    const int steps = static_cast<int>(durationSeconds / kDt + 0.5);

    for (int i = 0; i < steps; ++i) {
        aircraft.update(kDt);

        if (i % kLogLevelEverySteps == 0) {
            LogStep(startTimeSeconds + i * kDt, aircraft);
        }
    }

    LogStep(startTimeSeconds + durationSeconds, aircraft);
}

/*
Phase commune aux scenarios aeriens : montee rapide pour prendre de l'altitude.
Retourne l'instant de fin de la phase.
*/
double TakeOff(Aircraft& aircraft, double hoverRpm)
{
    aircraft.set_command({1.3 * hoverRpm, 0.0, 0.0});
    Run(aircraft, 0.0, 3.0);
    return 3.0;
}