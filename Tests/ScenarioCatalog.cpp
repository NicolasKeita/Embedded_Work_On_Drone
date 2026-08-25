/*
Filename: Tests/ScenarioCatalog.cpp
Description: Definition of the ScenarioCatalog static members and methods.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module ScenarioCatalog;

import std;

import FlightMission;
import FlightScenarios;
import Scenarios;
import TestHarness;

namespace sim::test {

const std::array<ScenarioEntry, 10> ScenarioCatalog::scenarios_{{
    {'a', "Repos (RPM = 0, servos = 0)", scenarios::rest},
    {'b', "Montee (RPM > hover)", scenarios::climb},
    {'c', "Descente (RPM < hover)", scenarios::descent},
    {'d', "Deplacement X (hover + pitch > 0)", scenarios::move_x},
    {'e', "Deplacement Y (hover + roll > 0)", scenarios::move_y},
    {'f', "Combine (RPM > hover, pitch > 0, roll < 0)", scenarios::combined},
    {'g', "Autonomie : altitude pure (z -> 100 m)", flight_scenarios::autonomous_altitude},
    {'h', "Autonomie : axe X en cascade (x 20 -> 0)", flight_scenarios::autonomous_position_x},
    {'i', "Autonomie : axe Y en cascade (y -15 -> 0)", flight_scenarios::autonomous_position_y},
    {'j', "Autonomie : mission complete (TAKEOFF a COMPLETE)", flight_scenarios::autonomous_mission},
}};

std::span<const ScenarioEntry> ScenarioCatalog::all() noexcept
{
    return scenarios_;
}

const ScenarioEntry* ScenarioCatalog::find(char argument) noexcept
{
    char normalizedKey = argument;

    if (normalizedKey >= 'A' && normalizedKey <= 'Z') {
        normalizedKey = static_cast<char>(normalizedKey + ('a' - 'A'));
    }

    for (const ScenarioEntry& entry : scenarios_) {
        if (entry.key == normalizedKey) {
            return &entry;
        }
    }

    return nullptr;
}

void ScenarioCatalog::print_usage(std::string_view executableName)
{
    std::cout << "Validation du simulateur physique (Heliblade-like)." << std::endl;
    std::cout << "Utilisation : " << executableName << " [scenario ...]" << std::endl;
    std::cout << "  Sans argument : tous les scenarios sont executes." << std::endl;
    std::cout << "  scenario      : lettre(s) parmi";
    for (const ScenarioEntry& entry : scenarios_) {
        std::cout << ' ' << entry.key;
    }
    std::cout << " (insensible a la casse), ou -h / --help." << std::endl;
    std::cout << std::endl;
    std::cout << "Scenarios disponibles :" << std::endl;

    for (const ScenarioEntry& entry : scenarios_) {
        std::cout << "  " << entry.key << " : " << entry.description << std::endl;
    }
}

}
