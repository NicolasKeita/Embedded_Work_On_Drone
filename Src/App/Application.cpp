/*
Filename: Src/App/Application.cpp
Description: Application runtime executing the aircraft physics simulation demo.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module App;

import std;

import PhysicsModel;
import Sensors;
import Actuators;
import Aircraft;

void Application::PrintTelemetry(double timeSeconds, const Aircraft& aircraft)
{
    std::cout << "t = " << std::fixed << std::setprecision(1) << timeSeconds
              << "s   altitude = " << std::setprecision(2)
              << aircraft.GetSensors().GetAltitudeMeters() << "m   vitesse verticale = "
              << aircraft.GetSensors().GetVerticalSpeedMps() << "m/s" << std::endl;
}

int Application::RunSimulationDemo() const
{
    std::cout << "\n=== Demo physique : RPM = 1000, servos a 0 degre ===" << std::endl;

    Aircraft aircraft;
    aircraft.SetCommand({1000.0, 0.0, 0.0});

    for (int second = 0; second <= 8; ++second)
    {
        if (second == 5)
        {
            std::cout << "--- On baisse les tours moteur : RPM 1000 -> 500 ---" << std::endl;
            aircraft.SetCommand({500.0, 0.0, 0.0});
        }

        PrintTelemetry(static_cast<double>(second), aircraft);

        for (int step = 0; step < 100; ++step)
        {
            aircraft.Update(0.01);
        }
    }

    return 0;
}

int Application::Run() const
{
    return RunSimulationDemo();
}
