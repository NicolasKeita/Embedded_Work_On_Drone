/*
Filename: Tests/Scenarios/Scenarios-Catalog.cpp
Description: Definition of the ScenarioCatalog static members and methods. Shared
scenarios take their canonical description from the FunctionalScenarios
registry; simulation-only scenarios keep their local description (single
definition, they exist in no other catalog).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module Scenarios;

import std;

import FlightScenarios;
import FunctionalScenarios;
import TestHarness;

namespace sim::test {

namespace {

/*
Canonical description of a scenario registered in the shared FunctionalScenarios
registry. Simulation-only scenarios keep their local literal, which is then
their single definition.
*/
std::string_view shared_description(std::string_view id)
{
    const FunctionalScenario* shared = find_functional_scenario(id);
    return shared != nullptr ? shared->description : std::string_view{};
}

}

const std::array<ScenarioEntry, 17> ScenarioCatalog::scenarios_{{
    {"NOMINAL-001",        shared_description("NOMINAL-001"),     flight_scenarios::autonomous_altitude_hold},
    {"NOMINAL-002",         "Grounded rest (RPM = 0, servos = 0)",         scenarios::rest},
    {"NOMINAL-003",        "Vertical climb (RPM > hover)",              scenarios::climb},
    {"NOMINAL-004",             "Descent (RPM < hover)",                     scenarios::descent},
    {"NOMINAL-005",  "Forward translation (hover + pitch > 0)",    scenarios::move_x},
    {"NOMINAL-006",  "Lateral translation (hover + roll > 0)",     scenarios::move_y},
    {"NOMINAL-007", "Combined translation (RPM > hover, "
                                    "pitch > 0, roll < 0)",                      scenarios::combined},
    {"NOMINAL-008",        "Autonomous cascaded X axis (x: 20 -> 0)",
     flight_scenarios::autonomous_position_x},
    {"NOMINAL-009",        "Autonomous cascaded Y axis (y: -15 -> 0)",
     flight_scenarios::autonomous_position_y},
    {"NOMINAL-010",        "Autonomous full mission (TAKEOFF to COMPLETE)",
     flight_scenarios::autonomous_mission},
    {"NOMINAL-011",        "Autonomous altitude hold (z: 0 -> 100 m)",
     flight_scenarios::autonomous_altitude},
    {"NOMINAL-012", shared_description("NOMINAL-012"),
     flight_scenarios::low_vertical_takeoff},
    {"NOMINAL-013", shared_description("NOMINAL-013"),
     flight_scenarios::low_forward_takeoff},
    {"NOMINAL-014", shared_description("NOMINAL-014"),
     flight_scenarios::low_lateral_takeoff},
    {"NOMINAL-015", shared_description("NOMINAL-015"),
     flight_scenarios::low_diagonal_takeoff},
    {"NOMINAL-016", shared_description("NOMINAL-016"),
     flight_scenarios::low_offset_takeoff},
    {"NOMINAL-017", shared_description("NOMINAL-017"),
     flight_scenarios::stratosphere_climb},
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
