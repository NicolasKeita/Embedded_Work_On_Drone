/*
Filename: Src/SIL/Validation/FlightControlValidation-Runner.cpp
Description: Monte-Carlo orchestration : per-run SIL execution and root-cause classification.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilRunner;
import SilTypes;

namespace sim::sil::validation {

MonteCarloRunner::MonteCarloRunner(std::uint64_t master_seed, SilConfig config)
    : generator_(master_seed)
    , config_(std::move(config))
{
}

std::uint64_t MonteCarloRunner::master_seed() const noexcept
{
    return generator_.master_seed();
}

const ResultCollector& MonteCarloRunner::collector() const noexcept
{
    return collector_;
}

ResultCollector& MonteCarloRunner::collector() noexcept
{
    return collector_;
}

/*
Builds a per-run SilConfig carrying the sampled duration, time step and seed so
that the SIL engine itself stays deterministic and independent of every other
run.
*/
static SilConfig make_run_config(const SilConfig& base, const Scenario& scenario)
{
    SilConfig config = base;

    config.duration_s = scenario.simulation_duration_s;
    config.dt = scenario.time_step_s;
    config.target.z = scenario.target_altitude_m;
    config.seed = scenario.scenario_seed;
    return config;
}

/*
Executes one run: projects the scenario onto a FaultScenario, drives a fresh
SILRunner, then classifies and stores the structured outcome. Runner errors are
recorded with FailureReason::RunnerError rather than propagated as exceptions.
*/
void MonteCarloRunner::execute_run(std::uint64_t run_id)
{
    Scenario scenario = generator_.generate_scenario(run_id);
    const SilConfig config = make_run_config(config_, scenario);
    const FaultScenario fault = scenario.to_fault_scenario();
    const bool fault_expected = (scenario.fault_type != FaultType::None);

    SimulationResult result{.run_id = run_id, .scenario_seed = scenario.scenario_seed, .scenario = scenario};

    SILRunner runner(config);
    std::array<FaultScenario, 1> scenarios{ fault };
    auto outcome = runner.run(std::span<const FaultScenario>{ scenarios });

    if (!outcome.has_value()) {
        result.failure_reason = FailureReason::RunnerError;
        result.verdict = false;
        collector_.record(std::move(result));
        return;
    }

    result.sil_result = outcome->result;
    result = classify(scenario, result.sil_result);
    result.run_id = run_id;
    result.scenario_seed = scenario.scenario_seed;
    result.scenario = scenario;
    result.verdict = (result.failure_reason == FailureReason::None) &&
                     result.sil_result.compute_verdict(fault_expected);
    collector_.record(std::move(result));
}

/*
Drives N deterministic runs. The per-run work is delegated to execute_run so
that this function stays a thin loop over the run ids.
*/
CampaignSummary MonteCarloRunner::run(std::uint64_t run_count)
{
    collector_.clear();
    collector_.reserve(run_count);

    for (std::uint64_t run_id = 0; run_id < run_count; ++run_id) {
        execute_run(run_id);
    }

    return collector_.summarize();
}

}
