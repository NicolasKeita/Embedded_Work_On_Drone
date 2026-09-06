/*
Filename: Src/SIL/Core/FunctionalScenarios.cppm
Description: Target-agnostic functional scenario registry. Each entry binds a
standardised scenario ID (NOMINAL-xxx, FAULT_INJECTOR-xxx, MONTE_CARLO-xxx,
MONTE_CARLO_FAULT_INJECTOR-xxx) to its declarative fault identity (type and
parameters) and expected outcome, shared by the SIL and HIL runners.
The execution target (SIL/HIL) is injected at runtime by the harness, so the
scenario names never encode the execution environment.
Exports:
    enum class FunctionalFamily,
    struct FunctionalScenario,
    functional_scenarios(),
    find_functional_scenario()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FunctionalScenarios;

import std;

import SilFaultScenario;

export namespace sim::test {

enum class FunctionalFamily : std::uint8_t {
    Nominal,
    FaultInjection,
    MonteCarlo,
    MonteCarloFaultInjection
};

/*
Target-agnostic description of one functional scenario: its standardised ID,
human-readable description, the declarative fault identity (type and parameters;
FaultType::None for nominal scenarios), whether a fault is expected, and the
family used to group the scenario in the taxonomy. The fault activation timing
(start time and duration) is target-specific and applied by each runner.
*/
struct FunctionalScenario {
    std::string_view          id;
    std::string_view          description;
    FunctionalFamily          family;
    sim::sil::FaultType       fault_type;
    sim::sil::FaultParameters parameters;
    bool                      fault_expected;
};

[[nodiscard]] std::span<const FunctionalScenario> functional_scenarios() noexcept;

/*
Resolves a functional scenario by its standardised ID (NOMINAL-xxx,
FAULT_INJECTOR-xxx, MONTE_CARLO-xxx, MONTE_CARLO_FAULT_INJECTOR-xxx); returns
nullptr when the ID is unknown.
*/
[[nodiscard]] const FunctionalScenario* find_functional_scenario(std::string_view id) noexcept;

}
