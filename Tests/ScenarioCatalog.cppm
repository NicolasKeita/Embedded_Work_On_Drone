/*
Filename: Tests/ScenarioCatalog.cppm
Description: Public interface of the launchable scenario registry and command-line usage.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ScenarioCatalog;

import std;

import TestHarness;

export struct ScenarioEntry
{
    char key;
    const char* description;
    void (*run)(sim::test::TestRunner&, double);
};

export [[nodiscard]] const std::array<ScenarioEntry, 6>& GetScenarios();
export [[nodiscard]] const ScenarioEntry* FindScenario(char argument);
export void PrintUsage(const char* executableName);