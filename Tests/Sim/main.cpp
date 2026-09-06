/*
Filename: Tests/Sim/main.cpp
Description: Entry point running the deterministic validation scenarios at 100 Hz.
The command-line parsing (scenario selection, --target) lives in the SimCli module.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import Scenarios;
import SimCli;
import TestHarness;

/*
Entry point: a single runner is shared by all scenarios so that the failure
counter is accumulated over the whole validation.
*/
int main(int argc, char* argv[])
{
    const std::string_view       executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "test_simulation";
    sim::test::ScenarioSelection selected;
    sim::test::RunTarget         parsedTarget = sim::test::RunTarget::Simulation;
    const std::int32_t           earlyStatus =
        sim::test::select_scenarios(argc, argv, executableName, selected, parsedTarget);

    if (earlyStatus >= 0) {
        return static_cast<int>(earlyStatus);
    }

    const Aircraft reference;
    const std::float64_t hoverRpm = reference.hover_rpm();

    sim::test::TestHarness runner{sim::test::HarnessConfig{
        .dt = 0.01, .log_interval_steps = 100, .target = parsedTarget}};

    std::cout << "=== Physical simulator validation (Heliblade-like) ===" << std::endl;
    std::cout << "Theoretical hover RPM: "
              << std::fixed << std::setprecision(1) << hoverRpm << " rpm" << std::endl;
    std::cout << "Deterministic single-thread loop at 100 Hz (dt = 0.01 s)." << std::endl;

    for (std::size_t index = 0; index < selected.count; ++index) {
        selected.entries[index]->run(runner, hoverRpm);
    }

    if (runner.passed()) {
        std::cout << "\n>>> All launched scenarios passed (PASS)." << std::endl;
        return 0;
    }

    std::cout << "\n>>> " << runner.failure_count()
              << " verification(s) failed (FAIL)." << std::endl;
    return 1;
}
