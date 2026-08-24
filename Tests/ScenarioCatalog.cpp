/*
Filename: Tests/ScenarioCatalog.cpp
Description: Registry of the launchable validation scenarios and command-line usage.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioCatalog;

import std;

import Scenarios;

namespace
{
    const std::array<ScenarioEntry, 6> kScenarios{{
        {'a', "Repos (RPM = 0, servos = 0)", ScenarioRest},
        {'b', "Montee (RPM > hover)", ScenarioClimb},
        {'c', "Descente (RPM < hover)", ScenarioDescent},
        {'d', "Deplacement X (hover + pitch > 0)", ScenarioMoveX},
        {'e', "Deplacement Y (hover + roll > 0)", ScenarioMoveY},
        {'f', "Combine (RPM > hover, pitch > 0, roll < 0)", ScenarioCombined},
    }};
}

const std::array<ScenarioEntry, 6>& GetScenarios()
{
    return kScenarios;
}

const ScenarioEntry* FindScenario(char argument)
{
    char normalizedKey = argument;

    if (normalizedKey >= 'A' && normalizedKey <= 'Z') {
        normalizedKey = static_cast<char>(normalizedKey + ('a' - 'A'));
    }

    for (const ScenarioEntry& entry : kScenarios) {
        if (entry.key == normalizedKey) {
            return &entry;
        }
    }

    return nullptr;
}

void PrintUsage(const char* executableName)
{
    std::cout << "Validation du simulateur physique (Heliblade-like)." << std::endl;
    std::cout << "Utilisation : " << executableName << " [scenario ...]" << std::endl;
    std::cout << "  Sans argument : tous les scenarios sont executes." << std::endl;
    std::cout << "  scenario      : lettre(s) parmi";
    for (const ScenarioEntry& entry : kScenarios) {
        std::cout << ' ' << entry.key;
    }
    std::cout << " (insensible a la casse), ou -h / --help." << std::endl;
    std::cout << std::endl;
    std::cout << "Scenarios disponibles :" << std::endl;

    for (const ScenarioEntry& entry : kScenarios) {
        std::cout << "  " << entry.key << " : " << entry.description << std::endl;
    }
}