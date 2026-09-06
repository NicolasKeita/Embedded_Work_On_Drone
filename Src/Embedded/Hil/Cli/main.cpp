/*
Filename: Src/Embedded/Hil/Cli/main.cpp
Description: Entry point of hil_runner : loads a HIL scenario, prints the run header,
executes the real-time closed-loop mission against the host FC emulator with live
telemetry/event streaming to the terminal, then prints the post-run summary report.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import HilConfig;
import HilReport;
import HilRunner;
import HilRunnerCli;
import HilRunnerTypes;
import SilFaultScenario;

int main(int argc, char** argv)
{
    const sim::hil::HilCliOptions options = sim::hil::parse_hil_cli(argc, argv);

    if (options.help) {
        sim::hil::print_hil_usage(argv[0]);
        return 0;
    }
    if (options.list_only) {
        sim::hil::list_hil_scenarios();
        return 0;
    }

    const sim::hil::HilConfig config = sim::hil::hil_config_from_options(options);
    const std::array<sim::sil::FaultScenario, 1> scenarios{sim::hil::hil_fault_from_options(options)};
    const bool fault_expected = scenarios[0].fault_type != sim::sil::FaultType::None;

    sim::hil::write_header(std::cout, config, fault_expected);

    sim::hil::HilRunner runner{config};
    runner.setLiveStream(std::cout);
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> outcome = runner.run(scenarios);
    if (!outcome.has_value()) {
        std::cerr << "HIL run failed\n";
        return 1;
    }

    sim::hil::write_summary(std::cout, *outcome);
    return (*outcome).result.test_verdict ? 0 : 1;
}
