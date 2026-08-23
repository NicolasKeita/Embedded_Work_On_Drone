/*
Filename: Src/App/Application.cppm
Description: Public application interface running the aircraft physics demo.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module App;

import std;

import PhysicsModel;
import Sensors;
import Actuators;
import Aircraft;

export class Application
{
public:
    int Run() const;

private:
    static void PrintTelemetry(double timeSeconds, const Aircraft& aircraft);
    int RunSimulationDemo() const;
};
