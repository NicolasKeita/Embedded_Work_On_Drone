/*
Filename: Tests/Scenarios/Scenarios-Catalog.cpp
Description: Definition of the ScenarioCatalog static members and methods.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Scenarios;

import std;

import FlightScenarios;
import TestHarness;

namespace sim::test {

const std::array<ScenarioEntry, 10> ScenarioCatalog::scenarios_{{
    {"NOM-002_GroundedRest",         "Grounded rest (RPM = 0, servos = 0)",         scenarios::rest},
    {"NOM-003_VerticalClimb",        "Vertical climb (RPM > hover)",              scenarios::climb},
    {"NOM-004_Descent",             "Descent (RPM < hover)",                     scenarios::descent},
    {"NOM-005_ForwardTranslation",  "Forward translation (hover + pitch > 0)",    scenarios::move_x},
    {"NOM-006_LateralTranslation",  "Lateral translation (hover + roll > 0)",     scenarios::move_y},
    {"NOM-007_CombinedTranslation", "Combined translation (RPM > hover, "
                                    "pitch > 0, roll < 0)",                      scenarios::combined},
    {"MC-001_AltitudeHold",         "Autonomous altitude hold (z: 0 -> 100 m)",  flight_scenarios::autonomous_altitude},
    {"MC-002_PositionXHold",        "Autonomous cascaded X axis (x: 20 -> 0)",
                                    flight_scenarios::autonomous_position_x},
    {"MC-003_PositionYHold",        "Autonomous cascaded Y axis (y: -15 -> 0)",
                                    flight_scenarios::autonomous_position_y},
    {"MC-004_FullMission",          "Autonomous full mission (TAKEOFF to COMPLETE)",
                                    flight_scenarios::autonomous_mission},
}};

std::span<const ScenarioEntry> ScenarioCatalog::all() noexcept
{
    return scenarios_;
}

const ScenarioEntry* ScenarioCatalog::find(std::string_view id) noexcept
{
    for (const ScenarioEntry& entry : scenarios_) {
        if (entry.id == id) {
            return &entry;
        }
    }

    return nullptr;
}

void ScenarioCatalog::print_usage(std::string_view executableName)
{
    std::cout << "Physical simulator validation (Heliblade-like)." << std::endl;
    std::cout << "Usage: " << executableName << " [--target=simulation] [scenario ...]" << std::endl;
    std::cout << "  With no scenario argument, all scenarios are executed (in catalog order)." << std::endl;
    std::cout << "  --target=simulation  Execution target (default: simulation, also: sim)." << std::endl;
    std::cout << "  -h, --help            Show this help." << std::endl;
    std::cout << std::endl;
    std::cout << "Available scenarios:" << std::endl;

    for (const ScenarioEntry& entry : scenarios_) {
        std::cout << "  " << entry.id << " : " << entry.description << std::endl;
    }
}

}
