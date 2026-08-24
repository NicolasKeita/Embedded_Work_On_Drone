/*
Filename: Tests/ScenarioCatalog.cpp
Description: Registry of the launchable validation scenarios and command-line usage.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioCatalog;

import std;

import Scenarios;
import TestHarness;

namespace
{
    const std::array kScenarios{
        sim::test::ScenarioEntry{'a', "Repos (RPM = 0, servos = 0)", sim::test::scenarios::rest},
        sim::test::ScenarioEntry{'b', "Montee (RPM > hover)", sim::test::scenarios::climb},
        sim::test::ScenarioEntry{'c', "Descente (RPM < hover)", sim::test::scenarios::descent},
        sim::test::ScenarioEntry{'d', "Deplacement X (hover + pitch > 0)", sim::test::scenarios::move_x},
        sim::test::ScenarioEntry{'e', "Deplacement Y (hover + roll > 0)", sim::test::scenarios::move_y},
        sim::test::ScenarioEntry{'f', "Combine (RPM > hover, pitch > 0, roll < 0)", sim::test::scenarios::combined},
    };
}

namespace sim::test {

std::span<const ScenarioEntry> get_scenarios() noexcept
{
    return kScenarios;
}

const ScenarioEntry* find_scenario(char argument) noexcept
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

void print_usage(std::string_view executableName)
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

}
