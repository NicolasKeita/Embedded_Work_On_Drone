/*
Filename: Tests/Sil/SilScenarios-Suite.cpp
Description: SIL suite catalog binding the canonical scenarios of the shared
FunctionalScenarios registry to SIL execution functions and SIL fault timing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import FunctionalScenarios;
import SilFaultScenario;
import TestHarness;

namespace sim::test::sil {

void steady_wind_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void gust_wind_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void nominal_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void fc1_failure_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void sensor_fault_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);

/* Canonical description of a registered scenario (empty view when the ID is unknown). */
std::string_view shared_description(std::string_view id)
{
    const sim::test::FunctionalScenario* shared = sim::test::find_functional_scenario(id);

    return shared == nullptr ? std::string_view{} : shared->description;
}

/* Applies SIL timing to a fault identity owned by the shared functional registry. */
sim::sil::FaultScenario make_sil_fault(std::string_view id,
                                       std::float64_t   start_time,
                                       std::float64_t   duration)
{
    const sim::test::FunctionalScenario* shared = sim::test::find_functional_scenario(id);

    return {
        .start_time = start_time,
        .duration = duration,
        .failure_mode = shared->failure_mode,
        .parameters = shared->parameters,
    };
}

/* SIL execution bindings for the canonical scenarios defined by FunctionalScenarios. */
const std::array<SilScenarioEntry, 5> kSilScenarios{{
    {"NOMINAL-001",        shared_description("NOMINAL-001"),        nominal_scenario},
    {"FAULT_INJECTOR-001", shared_description("FAULT_INJECTOR-001"), fc1_failure_scenario},
    {"FAULT_INJECTOR-003", shared_description("FAULT_INJECTOR-003"), sensor_fault_scenario},
    {"WIND-001", shared_description("WIND-001"), steady_wind_scenario},
    {"WIND-002", shared_description("WIND-002"), gust_wind_scenario},
}};

std::span<const SilScenarioEntry> sil_scenarios() noexcept
{
    return kSilScenarios;
}

const SilScenarioEntry* find_sil_scenario(std::string_view id) noexcept
{
    for (const SilScenarioEntry& entry : kSilScenarios) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

}
