/*
Filename: Src/SIL/Validation/FlightControlValidation-Generator.cpp
Description: Deterministic scenario generation : seed mixing and RNG sampling of Scenario fields.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilTypes;

namespace sim::sil::validation {

/*
Fixed 64-bit mixing function (SplitMix64 derivative) combining the master seed
with the run id. The constant is the odd multiplier recommended by the
Hash127 family so that the avalanche is uniform and the sequence of sub-seeds
is exactly reproducible across platforms and toolchains.
*/
std::uint64_t ScenarioGenerator::mix_seed(std::uint64_t master_seed, std::uint64_t run_id)
{
    std::uint64_t z = master_seed ^ (run_id + 0x9E3779B97F4A7C15ULL);

    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

ScenarioGenerator::ScenarioGenerator(std::uint64_t master_seed)
    : master_seed_(master_seed)
{
}

std::uint64_t ScenarioGenerator::master_seed() const noexcept
{
    return master_seed_;
}

/*
Samples every field of one Scenario from the per-run sub-seed. The fault family
is drawn first so that the rest of the sampling can branch on it. Distributions
are scoped locally so that no shared state leaks between runs and the
generation stays allocation-free.
*/
Scenario ScenarioGenerator::generate_scenario(std::uint64_t run_id, std::optional<FaultType> fault_template)
{
    const std::uint64_t seed = mix_seed(master_seed_, run_id);
    std::mt19937_64     generator(seed);
    Scenario            scenario{.run_id = run_id, .scenario_seed = seed};

    sample_fault_window(scenario, generator, fault_template);
    sample_fault_parameters(scenario, generator);
    sample_environment(scenario, generator);
    sample_initial_conditions(scenario, generator);

    return scenario;
}

}
