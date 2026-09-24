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
import ConfigFile;
import FunctionalScenarios;
import ScenarioConfig;
import SimulationConfig;
import SilRuntimeConfig;
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
                     std::string_view                  executableName)
    {
        if (options.scenario.has_value()) {
            const std::string& scenarioId = *options.scenario;
            if (sim::test::sil::find_sil_scenario(scenarioId) != nullptr) {
                sim::test::sil::run_sil_scenario(scenarioId, runner, sim::host::sil_report_period(scenarioId));
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

    const auto simulation = sim::host::load_simulation_config(options.simulation_config_path);
    if (!simulation) {
        std::cerr << "Error: " << simulation.error() << std::endl;
        return 2;
    }
    auto runtime = sim::host::load_sil_runtime_config(options.config_path);
    if (!runtime) {
        std::cerr << "Error: " << runtime.error() << std::endl;
        return 2;
    }
    const auto scenarios = sim::host::load_scenario_configs(options.scenarios_dir);
    if (!scenarios) {
        std::cerr << "Error: " << scenarios.error() << std::endl;
        return 2;
    }
    if (options.telemetry_period.has_value()) {
        runtime->report_period_s = *options.telemetry_period;
        runtime->report_period_cli = true;
    }
    runtime->verbose = options.verbose;
    if (options.verbose) {
        runtime->report_period_s = runtime->runner.dt;
        runtime->report_period_cli = true;
    }
    if (runtime->report_period_s < runtime->runner.dt) {
        std::cerr << "Error: telemetry report period must be at least dt_s." << std::endl;
        return 2;
    }
    const auto validated = sim::host::validate_sil_runtime_profiles(*runtime, options.scenario.value_or(""));
    if (!validated) {
        std::cerr << "Error: " << validated.error() << std::endl;
        return 2;
    }
    sim::host::set_sil_runtime_options(*runtime);
    const std::float64_t effective_report_period_s = options.scenario.has_value()
        ? sim::host::sil_report_period(*options.scenario) : runtime->report_period_s;
    std::ostringstream overrides;
    overrides << std::setprecision(17)
              << "runner = sil\nscenario = " << options.scenario.value_or("all")
              << "\neffective.report_period_s = " << effective_report_period_s
              << "\nderived.controller.hover_rpm = " << runtime->runner.controller.hover_rpm
              << "\ncli.report_period_override = " << std::boolalpha << runtime->report_period_cli
              << "\ncli.verbose = " << runtime->verbose << '\n';
    if (options.scenario.has_value()
        && sim::test::find_functional_scenario(*options.scenario) != nullptr) {
        const auto effective = sim::host::sil_config_for_scenario(*options.scenario);
        overrides << "effective.dt_s = " << effective.dt
                  << "\neffective.duration_s = " << effective.duration_s
                  << "\neffective.seed = " << effective.seed
                  << "\neffective.target_x_m = " << effective.target.x
                  << "\neffective.target_y_m = " << effective.target.y
                  << "\neffective.target_z_m = " << effective.target.z
                  << "\neffective.controller.station_hold_seconds = " << effective.controller.station_hold_seconds
                  << "\neffective.sensor_max_altitude_m = " << effective.sensor_limits.max_altitude_m << '\n';
    }
    const auto snapshot = sim::config::write_configuration_snapshot(options.config_output, overrides.str());
    if (!snapshot) {
        std::cerr << "Error: " << snapshot.error() << std::endl;
        return 2;
    }
    const std::size_t logIntervalSteps = std::max(std::size_t{1},
        static_cast<std::size_t>(runtime->report_period_s / runtime->runner.dt + 0.5));
    sim::test::HarnessConfig harnessConfig{
        .dt = runtime->runner.dt,
        .log_interval_steps = logIntervalSteps,
        .target = sim::test::RunTarget::SIL};
    sim::test::TestHarness runner{harnessConfig};

    const int outcome = execute_runs(options, runner, executableName);
    if (outcome != 0) {
        return outcome;
    }
    return print_verdict(options, runner);
}
