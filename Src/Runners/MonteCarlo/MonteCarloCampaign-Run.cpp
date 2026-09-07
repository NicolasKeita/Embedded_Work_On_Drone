/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Run.cpp
Description: Dispersed scenario execution and campaign loop of the Monte-Carlo run.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

import Aircraft;
import PhysicsDispersion;
import Scenarios;
import TestHarness;

namespace sim::monte_carlo {

namespace {
    /* Executes one dispersed scenario run and collects its tracking metrics. */
    RunMetrics execute_scenario_run(const std::string&            scenario_id,
                                    const sim::PhysicsDispersion& dispersion,
                                    bool                          verbose)
    {
        RunMetrics                      metrics{};
        const sim::test::ScenarioEntry* scenario_entry = sim::test::ScenarioCatalog::find(scenario_id);

        if (scenario_entry == nullptr) {
            std::cerr << "Error: Unknown scenario " << scenario_id << std::endl;
            metrics.passed = false;
            return metrics;
        }
        sim::test::TestHarness runner{sim::test::HarnessConfig{.verbose = verbose}};
        const Aircraft reference{dispersion};
        scenario_entry->run(runner, reference.hover_rpm(), dispersion);
        metrics.passed = runner.passed();
        metrics.overshoot = runner.overshoot();
        metrics.settling_time = runner.settling_time();
        metrics.steady_state_error = runner.steady_state_error();
        metrics.max_acceleration = runner.max_acceleration();
        return metrics;
    }

    /* Accumulates one run outcome into the campaign statistics. */
    void record_run(CampaignStats& stats, const RunInputs& inputs, const RunMetrics& run_metrics)
    {
        if (run_metrics.passed) {
            ++stats.passed_runs;
        }
        else {
            ++stats.failed_runs;
        }
        stats.records.push_back(RunRecord{.inputs = inputs, .metrics = run_metrics});
    }
}

/*
Executes the campaign: one dispersed scenario run per iteration, capturing the
perturbed inputs and accumulating the per-run records.
*/
CampaignStats run_campaign(const CliOptions& options)
{
    CampaignStats            stats{.master_seed = options.seed, .total_runs = options.runs};
    sim::DispersionGenerator dispersion_generator(options.seed);
    stats.records.reserve(options.runs);

    for (std::uint32_t run_index = 0; run_index < options.runs; ++run_index) {
        const std::uint64_t          run_seed = dispersion_generator.generate_run_seed(options.seed, run_index);
        const sim::PhysicsDispersion dispersion = dispersion_generator.generate_run_dispersion(run_index);
        const RunInputs              inputs{.run_id = run_index,
                                            .master_seed = options.seed,
                                            .run_seed = run_seed,
                                            .dispersion = dispersion};
        if (options.verbose) {
            print_run_inputs(inputs, options.runs);
        }
        const RunMetrics run_metrics = execute_scenario_run(options.scenario, dispersion, options.verbose);
        record_run(stats, inputs, run_metrics);
        if (!options.verbose && !run_metrics.passed) {
            print_run_inputs(inputs, options.runs);
        }
        if ((run_index + 1) % 10 == 0 || run_index + 1 == options.runs) {
            std::cout << "Completed run " << (run_index + 1) << "/" << options.runs << std::endl;
        }
    }
    return stats;
}

}
