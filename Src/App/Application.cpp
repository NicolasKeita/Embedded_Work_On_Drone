/*
Filename: Src/App/Application.cpp
Description: Application runtime executing the aircraft physics simulation demo.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module App;

import std;

import Aircraft;

namespace {

constexpr double kDegreesPerRadian = 180.0 / std::numbers::pi;
constexpr int kTimePrecision = 1;
constexpr int kValuePrecision = 2;

}

void Application::PrintTelemetry(double timeSeconds, const Aircraft& aircraft)
{
    const AircraftState& state = aircraft.state();
    const double pitchDegrees = state.pitch * kDegreesPerRadian;
    const double rollDegrees = state.roll * kDegreesPerRadian;

    std::cout << "t = " << std::fixed << std::setprecision(kTimePrecision) << timeSeconds
              << "s   z = " << std::setprecision(kValuePrecision) << state.z
              << "m   vz = " << state.vz
              << "m/s   pitch = " << pitchDegrees
              << "deg   roll = " << rollDegrees
              << "deg   rpm = " << state.actual_rpm << std::endl;
}

int Application::RunSimulationDemo() const
{
    const Aircraft reference;
    const double hoverRpm = reference.hover_rpm();

    std::cout << "\n=== Demo physique Heliblade-like ===" << std::endl;
    std::cout << "RPM de stationnaire theorique : "
              << std::fixed << std::setprecision(1) << hoverRpm << " tr/min" << std::endl;

    std::cout << "\n--- Montee : RPM = 1.2 x hover, servos a 0 degre ---" << std::endl;
    Aircraft aircraft;
    aircraft.set_command({1.2 * hoverRpm, 0.0, 0.0});

    for (std::uint32_t step = 0; step < 300; ++step) {
        aircraft.update(0.01);

        if (step % 50 == 0) {
            PrintTelemetry(step * 0.01, aircraft);
        }
    }

    std::cout << "--- Descente : RPM = 0.6 x hover ---" << std::endl;
    aircraft.set_command({0.6 * hoverRpm, 0.0, 0.0});

    for (std::uint32_t step = 0; step < 600; ++step) {
        aircraft.update(0.01);

        if (step % 100 == 0) {
            PrintTelemetry(3.0 + step * 0.01, aircraft);
        }
    }

    return 0;
}

int Application::Run() const
{
    return RunSimulationDemo();
}
