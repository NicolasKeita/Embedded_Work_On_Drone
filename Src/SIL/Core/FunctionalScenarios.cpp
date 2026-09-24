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
    std::size_t append_catalog(std::span<FunctionalScenario>       scenarios,
                               std::span<const FunctionalScenario> catalog,
                               std::size_t                         index)
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
        std::size_t                                               index = 0;

        index = append_catalog(scenarios, nominal_scenarios(), index);
        index = append_catalog(scenarios, fault_scenarios(), index);
        static_cast<void>(append_catalog(scenarios, wind_scenarios(), index));
        return scenarios;
    }

    /* Registry storage of the assembled catalogs, initialized on first use. */
    std::array<FunctionalScenario, functional_scenario_count>* registry_storage() noexcept
    {
        static std::array<FunctionalScenario, functional_scenario_count> scenarios =
            assemble_scenarios();

        return &scenarios;
    }
}

/* Returns the canonical profiles with any validated startup overrides applied. */
std::span<const FunctionalScenario> functional_scenarios() noexcept
{
    return std::span<const FunctionalScenario>{*registry_storage()};
}

/* Reconstructs the baseline from immutable catalogs instead of previously loaded values. */
std::array<FunctionalScenario, functional_scenario_count> functional_scenario_defaults() noexcept
{
    return assemble_scenarios();
}

/* Commits only profile values and retains stable literal-backed scenario identities. */
std::expected<void, std::string> apply_functional_scenarios(
    std::span<const FunctionalScenario> scenarios)
{
    std::array<FunctionalScenario, functional_scenario_count>& registry = *registry_storage();

    if (scenarios.size() != registry.size()) {
        return std::unexpected("Scenario configuration must preserve the complete fixed catalog");
    }
    for (std::size_t index = 0; index < registry.size(); ++index) {
        const FunctionalScenario& current = registry[index];
        const FunctionalScenario& replacement = scenarios[index];

        if (replacement.id != current.id || replacement.description != current.description
            || replacement.family != current.family || replacement.failure_mode != current.failure_mode
            || replacement.fault_expected != current.fault_expected
            || replacement.parameters.corruption != current.parameters.corruption) {
            return std::unexpected("Scenario configuration cannot change identity: " + std::string(current.id));
        }
    }
    for (std::size_t index = 0; index < registry.size(); ++index) {
        const std::string_view id = registry[index].id;
        const std::string_view description = registry[index].description;

        registry[index] = scenarios[index];
        registry[index].id = id;
        registry[index].description = description;
    }
    return {};
}

/* Resolves a stable canonical identity in the current startup-configured registry. */
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
