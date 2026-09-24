/*
Filename: Src/Runners/Hil/main.cpp
Description: Entry point of HIL_RUNNER : hardware-in-the-loop real-time test runner
(1 second of simulation time = 1 second of wall-clock time). Selects one scenario
with --scenario (NOMINAL-001, NOMINAL-012..017, FAULT_INJECTOR-001/003),
selects the hardware channel with --interface, or runs the deterministic HIL
validation suite with --selftest.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

#include <csignal>

import std;

import ConfigFile;
import HilConfig;
import HilReport;
import HilRunner;
import HilRunnerCli;
import HilRunnerTypes;
import HilRuntimeConfig;
import HilScenarios;
import HilTests;
import SilFaultScenario;
import TestHarness;

namespace
{
    volatile std::sig_atomic_t stop_requested = 0;

    /* Records a termination request so the HIL runner can restore the embedded targets. */
    void handle_stop_signal(int) noexcept
    {
        stop_requested = 1;
    }

    /* Runs the deterministic HIL validation suite (protocol, timing, faults, runner). */
    int run_selftest()
    {
        sim::test::HarnessConfig harnessConfig{.target = sim::test::RunTarget::HIL};
        sim::test::TestHarness   runner{harnessConfig};

        sim::test::hil::run_all_hil_tests(runner);

        if (runner.passed()) {
            std::cout << "\nAll HIL tests passed." << std::endl;
            return 0;
        }
        std::cout << "\n" << runner.failure_count() << " HIL test failure(s)." << std::endl;
        return 1;
    }

    /* Rejects an unknown scenario ID with the available catalog. */
    int reject_unknown_scenario(const std::string& scenarioId)
    {
        std::cout << "Error: unknown scenario \"" << scenarioId << "\"." << std::endl;
        std::cout << std::endl;
        sim::hil::list_hil_scenarios();
        return 2;
    }

}

/*
Entry point: loads the selected HIL scenario, prints the run header, executes
the real-time closed-loop mission against the host FC target with live
telemetry/event streaming, then prints the post-run summary report.
*/
int main(int argc, char** argv)
{
    const sim::hil::HilCliOptions options = sim::hil::parse_hil_cli(argc, argv);

    if (options.help || options.invalid) {
        sim::hil::print_hil_usage(argv[0]);
        return options.invalid ? 2 : 0;
    }
    if (options.list_only) {
        sim::hil::list_hil_scenarios();
        return 0;
    }
    if (options.selftest) {
        return run_selftest();
    }
    if (sim::hil::HilScenarioCatalog::find(options.scenario_id) == nullptr) {
        return reject_unknown_scenario(options.scenario_id);
    }

    const std::expected<sim::hil::HilConfig, std::string> configuration =
        sim::hil::hil_config_from_options(options);
    if (!configuration.has_value()) {
        std::cerr << "HIL configuration error: " << configuration.error() << "\n";
        return 2;
    }
    const sim::hil::HilConfig& config = *configuration;
    const std::array<sim::sil::FaultScenario, 1> scenarios{sim::hil::hil_fault_from_options(options)};
    const auto snapshot = sim::config::write_configuration_snapshot(
        options.config_output_path, sim::host::hil_configuration_text(config, scenarios[0]));
    if (!snapshot) {
        std::cerr << "HIL configuration error: " << snapshot.error() << "\n";
        return 2;
    }
    const bool fault_expected = scenarios[0].failure_mode != sim::sil::FailureMode::NONE;

    sim::hil::write_header(std::cout, config, fault_expected, sim::hil::detect_stm32_probe());

    sim::hil::HilRunner runner{config};
    runner.setLiveStream(std::cout);
    runner.setStopRequestedFlag(stop_requested);
    static_cast<void>(std::signal(SIGTERM, handle_stop_signal));
    static_cast<void>(std::signal(SIGINT, handle_stop_signal));
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> outcome = runner.run(scenarios);
    if (!outcome.has_value()) {
        std::cerr << "HIL run failed\n";
        return 1;
    }

    sim::hil::write_summary(std::cout, *outcome);
    return (*outcome).result.test_verdict ? 0 : 1;
}
