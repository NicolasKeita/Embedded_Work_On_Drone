/*
Filename: Tests/TestHarness.cppm
Description: Public interface of the shared validation harness (checks, logging, run loop).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module TestHarness;

import std;

import Aircraft;

export constexpr double kPi = 3.14159265358979323846;
export constexpr double kDt = 0.01;
export constexpr int kLogLevelEverySteps = 100;

export void Check(bool condition, const std::string& label);
export [[nodiscard]] int FailureCount();
export void LogHeader();
export void LogStep(double timeSeconds, const Aircraft& aircraft);
export void Run(Aircraft& aircraft, double startTimeSeconds, double durationSeconds);
export double TakeOff(Aircraft& aircraft, double hoverRpm);