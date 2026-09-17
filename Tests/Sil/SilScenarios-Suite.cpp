/*
Filename: Tests/Sil/SilScenarios-Suite.cpp
Description: SIL suite catalog for the scenarios shared with HIL.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import FunctionalScenarios;
import SilFaultScenario;
import TestHarness;

namespace sim::test::sil {

void nominal_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void fc1_failure_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);
void sensor_fault_scenario(TestHarness& runner, sim::sil::SilRunOutput& output, sim::sil::ScenarioRecord& record);

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
const std::array<SilScenarioEntry, 3> kSilScenarios{{
    {"NOMINAL-001",        "Nominal station-keeping mission (no fault)", nominal_scenario},
    {"FAULT_INJECTOR-001", "FC1 failure at 10 m, injected at t = 40.0 s",        fc1_failure_scenario},
    {"FAULT_INJECTOR-003", "Altitude sensor corruption at t = 20.0 s",  sensor_fault_scenario},
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
