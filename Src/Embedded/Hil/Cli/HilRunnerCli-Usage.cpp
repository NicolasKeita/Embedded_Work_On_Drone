/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Usage.cpp
Description: Usage banner and scenario listing of the HIL runner command-line interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

import HilScenarios;

namespace sim::hil {

void print_hil_usage(std::string_view name)
{
    std::cout << "HIL runner: real-time closed-loop mission against the FC target.\n";
    std::cout << "Usage: " << name << " [options]\n";
    std::cout << "  --scenario <id>         functional scenario ID (default NOMINAL-001)\n";
    std::cout << "  --interface <channel>   auto (trusted FC1, default), loopback or the trusted FC1 device\n";
    std::cout << "  --duration <s>          override mission duration in seconds\n";
    std::cout
        << "  --telemetry-period <s>  override human-readable report period (default 1 s)\n";
    std::cout << "  --seed <n>              random seed\n";
    std::cout << "  --noise <stddev>        sensor measurement noise std dev in meters\n";
    std::cout << "  --deadline <Warn|Fail|Abort>  deadline-miss policy (default Warn)\n";
    std::cout << "  --selftest              run the deterministic HIL validation suite\n";
    std::cout << "  --list                  list available scenarios\n";
}

void list_hil_scenarios()
{
    std::cout << "HIL scenarios:\n";
    for (const sim::hil::HilScenarioRecord& s : sim::hil::HilScenarioCatalog::all()) {
        std::cout << "  " << s.id << " : " << s.description << " [HIL]\n";
    }
}

}
