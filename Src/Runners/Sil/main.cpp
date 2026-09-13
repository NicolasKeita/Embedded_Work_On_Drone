/*
Filename: Src/Runners/Sil/main.cpp
Description: Entry point of SIL_RUNNER : deterministic software-in-the-loop test runner
executing at maximum CPU speed. With no argument the usage helper is printed; --scenario
selects one shared scenario (NOMINAL-001, FAULT_INJECTOR-001/003) and --all sweeps every
deterministic scenario.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import Scenarios;
import SilRunnerCli;
import SilScenarios;
import SilTwinViewer;
import TestHarness;

namespace
{
    /*
    Runs the requested scenario, or the whole deterministic sweep when --all is
    given. Returns 2 when the requested scenario is unknown.
    */
    int execute_runs(const sim::test::sil::CliOptions& options,
                     sim::test::TestHarness&           runner,
                     std::string_view                  executableName,
                     std::float64_t                    telemetryPeriodS)
    {
        if (options.scenario.has_value()) {
            const std::string& scenarioId = *options.scenario;
            if (sim::test::sil::find_sil_scenario(scenarioId) != nullptr) {
                sim::test::sil::run_sil_scenario(scenarioId, runner, telemetryPeriodS);
                return 0;
            }
            if (sim::test::ScenarioCatalog::find(scenarioId) != nullptr) {
                const Aircraft reference;
                sim::test::sil::run_functional_sil_twin(
                    *sim::test::ScenarioCatalog::find(scenarioId), runner, reference.hover_rpm());
                return 0;
            }
            std::cout << "Error: unknown scenario \"" << scenarioId << "\"." << std::endl;
            std::cout << std::endl;
            sim::test::sil::print_usage(executableName);
            return 2;
        }
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

    /* Prints the final verdict line (single scenario named or full sweep) and returns 0/1. */
    int print_verdict(const sim::test::sil::CliOptions& options, const sim::test::TestHarness& runner)
    {
        const std::string subject = options.scenario.has_value()
                                        ? "scenario " + *options.scenario : "all executed scenarios";

        std::cout << "\n>>> SIL_RUNNER: " << subject << ' ';
        if (runner.passed()) {
            std::cout << "passed." << std::endl;
            return 0;
        }
        std::cout << "failed (" << runner.failure_count() << " verification(s) failed)." << std::endl;
        return 1;
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
    if (!options.all && !options.scenario.has_value()) {
        sim::test::sil::print_usage(executableName);
        return 0;
    }

    constexpr std::float64_t kDefaultTelemetryPeriodS = 1.0;
    constexpr std::float64_t kSimulationTimeStepS     = 0.01;

    const std::float64_t telemetryPeriodS = options.telemetry_period.value_or(kDefaultTelemetryPeriodS);
    const std::size_t    logIntervalSteps
        = options.verbose ? std::size_t{1}
                          : static_cast<std::size_t>(telemetryPeriodS / kSimulationTimeStepS + 0.5);

    sim::test::HarnessConfig harnessConfig{
        .dt = kSimulationTimeStepS, .log_interval_steps = logIntervalSteps, .target = sim::test::RunTarget::SIL};
    sim::test::TestHarness runner{harnessConfig};

    const int outcome = execute_runs(options, runner, executableName, telemetryPeriodS);
    if (outcome != 0) {
        return outcome;
    }
    return print_verdict(options, runner);
}
