/*
Filename: Src/Runners/Sil/main.cpp
Description: Entry point of SIL_RUNNER : deterministic software-in-the-loop test runner
executing at maximum CPU speed. Selects one scenario with --scenario (engine-level
suite: NOMINAL-001, FAULT_INJECTOR-001..004; physics and autonomous catalog:
NOMINAL-002..010) or sweeps every deterministic scenario when none is given.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import Scenarios;
import SilRunnerCli;
import SilScenarios;
import TestHarness;

namespace
{
    /*
    Runs the requested scenario, or the whole deterministic sweep when none is
    given. Returns 2 when the requested scenario is unknown.
    */
    int execute_runs(const sim::test::sil::CliOptions& options,
                     sim::test::TestHarness&           runner,
                     std::string_view                  executableName)
    {
        if (!options.scenario.has_value()) {
            std::cout
                << "=== SIL_RUNNER deterministic sweep (software-in-the-loop, max CPU speed) ==="
                << std::endl;
            sim::test::sil::run_all_sil_scenarios(runner);
            std::cout << "\n=== Physics and autonomous functional scenarios ===" << std::endl;
            const Aircraft reference;
            for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
                entry.run(runner, reference.hover_rpm(), {});
            }
            return 0;
        }
        const std::string& scenarioId = *options.scenario;
        if (sim::test::sil::find_sil_scenario(scenarioId) != nullptr) {
            sim::test::sil::run_sil_scenario(scenarioId, runner);
            return 0;
        }
        if (sim::test::ScenarioCatalog::find(scenarioId) != nullptr) {
            const Aircraft reference;
            sim::test::ScenarioCatalog::find(scenarioId)->run(runner, reference.hover_rpm(), {});
            return 0;
        }
        std::cout << "Error: unknown scenario \"" << scenarioId << "\"." << std::endl;
        std::cout << std::endl;
        sim::test::sil::print_usage(executableName);
        return 2;
    }
}

/*
Entry point: a single harness is shared by every executed scenario so the
failure counter accumulates over the whole run; the exit code mirrors the
verdict (0 = all passed, 1 = at least one failure, 2 = command-line error).
*/
int main(int argc, char* argv[])
{
    const std::string_view           executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "SIL_RUNNER";
    const sim::test::sil::CliOptions options = sim::test::sil::parse_cli(argc, argv);

    if (options.help) {
        sim::test::sil::print_usage(executableName);
        return 0;
    }
    if (options.invalid) {
        std::cout << std::endl;
        sim::test::sil::print_usage(executableName);
        return 2;
    }

    const std::size_t logIntervalSteps = options.verbose ? std::size_t{1} : std::size_t{100};
    sim::test::HarnessConfig harnessConfig{
        .dt = 0.01, .log_interval_steps = logIntervalSteps, .target = sim::test::RunTarget::SIL};
    sim::test::TestHarness runner{harnessConfig};

    const int outcome = execute_runs(options, runner, executableName);
    if (outcome != 0) {
        return outcome;
    }
    if (runner.passed()) {
        std::cout << "\n>>> SIL_RUNNER: all executed scenarios passed." << std::endl;
        return 0;
    }
    std::cout << "\n>>> SIL_RUNNER: " << runner.failure_count() << " verification(s) failed." << std::endl;
    return 1;
}
