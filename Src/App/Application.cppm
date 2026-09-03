/*
Filename: Src/App/Application.cppm
Description: Public application interface running the aircraft physics demo.
Exports:
    class Application

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module App;

import std;

import Aircraft;

export class Application
{
public:
    std::int32_t Run() const;

private:
    static void PrintTelemetry(std::float64_t timeSeconds, const Aircraft& aircraft);
    std::int32_t RunSimulationDemo() const;
};
