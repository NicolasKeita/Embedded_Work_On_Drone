/*
Filename: Src/SIL/Core/FunctionalScenarios.cpp
Description: Assembly of the shared SIL/HIL functional scenario registry: concatenation of
the nominal, fault-injection and wind catalogs into the canonical order exposed to both
runners, plus the standardised ID lookup.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FunctionalScenarios;

import std;

namespace sim::test {

namespace {
    /* Copies one catalog into the assembled registry starting at the given index. */
    std::size_t append_catalog(std::span<FunctionalScenario> scenarios,
                               std::span<const FunctionalScenario> catalog,
                               std::size_t index)
    {
        for (const FunctionalScenario& scenario : catalog) {
            scenarios[index] = scenario;
            index = index + 1;
        }
        return index;
    }

    /* Concatenates the nominal, fault-injection and wind catalogs into the shared registry. */
    std::array<FunctionalScenario, functional_scenario_count> assemble_scenarios()
    {
        std::array<FunctionalScenario, functional_scenario_count> scenarios{{}};
        std::size_t index = 0;
        index = append_catalog(scenarios, nominal_scenarios(), index);
        index = append_catalog(scenarios, fault_scenarios(), index);
        static_cast<void>(append_catalog(scenarios, wind_scenarios(), index));
        return scenarios;
    }

    /* Registry storage of the assembled catalogs, initialized on first use. */
    const std::array<FunctionalScenario, functional_scenario_count>* registry_storage() noexcept
    {
        static const std::array<FunctionalScenario, functional_scenario_count> kScenarios =
            assemble_scenarios();

        return &kScenarios;
    }
}

std::span<const FunctionalScenario> functional_scenarios() noexcept
{
    return std::span<const FunctionalScenario>{*registry_storage()};
}

const FunctionalScenario* find_functional_scenario(std::string_view id) noexcept
{
    for (const FunctionalScenario& scenario : *registry_storage()) {
        if (scenario.id == id) {
            return &scenario;
        }
    }
    return nullptr;
}

}
