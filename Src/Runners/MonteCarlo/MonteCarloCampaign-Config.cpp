/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Config.cpp
Description: Host configuration loading and reproducibility snapshot of a Monte Carlo campaign.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

import ConfigFile;
import ScenarioConfig;
import Scenarios;
import SilRuntimeConfig;
import SimulationConfig;

namespace sim::monte_carlo {

/* Resolves all campaign configuration before generating any random samples. */
std::expected<CliOptions, std::string> prepare_campaign(CliOptions options)
{
    auto loaded = sim::config::load_config_file(options.config_path);
    if (!loaded) {
        return std::unexpected(loaded.error());
    }
    std::uint64_t seed = 42;
    std::uint64_t runs = 50;
    std::string scenario = "NOMINAL-001";
    loaded->unsigned_integer("seed", seed, 0, std::numeric_limits<std::uint64_t>::max());
    loaded->unsigned_integer("runs", runs, 1, 1000000);
    loaded->text("scenario", scenario);
    const auto campaign = loaded->finish();
    if (!campaign) {
        return std::unexpected(campaign.error());
    }
    if (!options.seed_override) {
        options.seed = seed;
    }
    if (!options.runs_override) {
        options.runs = static_cast<std::uint32_t>(runs);
    }
    if (!options.scenario_override) {
        options.scenario = scenario;
    }
    if (options.runs == 0 || options.runs > 1000000
        || sim::test::ScenarioCatalog::find(options.scenario) == nullptr) {
        return std::unexpected(std::string{"invalid Monte Carlo run count or unknown scenario"});
    }
    const auto simulation = sim::host::load_simulation_config(options.simulation_config_path);
    if (!simulation) {
        return std::unexpected(simulation.error());
    }
    const auto scenarios = sim::host::load_scenario_configs(options.scenarios_directory);
    if (!scenarios) {
        return std::unexpected(scenarios.error());
    }
    auto sil = sim::host::load_sil_runtime_config(options.sil_config_path);
    if (!sil) {
        return std::unexpected(sil.error());
    }
    sil->runner.seed = options.seed;
    sil->viewer_enabled = false;
    sil->verbose = options.verbose;
    const auto validated = sim::host::validate_sil_runtime_profiles(*sil, options.scenario);
    if (!validated) {
        return std::unexpected(validated.error());
    }
    sim::host::set_sil_runtime_options(*sil);
    const std::string runtime = std::format(
        "runner = SIL_MONTE_CARLO\nscenario = {}\nseed = {}\nruns = {}\ndt_s = {:.17g}\nviewer_enabled = false\n",
        options.scenario, options.seed, options.runs, sil->runner.dt);
    const auto snapshot = sim::config::write_configuration_snapshot(options.config_output_path, runtime);
    if (!snapshot) {
        return std::unexpected(snapshot.error());
    }
    return options;
}

}
