/*
Filename: Src/Runners/MonteCarlo/main.cpp
Description: Entry point of SIL_MONTE_CARLO : accelerated statistical batch simulation
over the SIL engine. --seed selects the master RNG seed, --runs the iteration count
and --scenario optionally restricts the campaign to one scenario template
(NOMINAL-001 or FAULT_INJECTOR-001..004). Results are exported as CSV.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import MonteCarloCampaign;
import ScenarioBrief;

/*
Entry point: parses the campaign options, executes the dispersed scenario runs
and prints the statistical summary of the whole campaign.
*/
int main(int argc, char* argv[])
{
    const std::string_view             executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "SIL_MONTE_CARLO";
    const sim::monte_carlo::CliOptions options = sim::monte_carlo::parse_cli(argc, argv);

    if (options.help) {
        sim::monte_carlo::print_usage(executableName);
        return 0;
    }
    if (options.invalid) {
        std::cout << std::endl;
        sim::monte_carlo::print_usage(executableName);
        return 2;
    }

    std::cout << "=== Monte-Carlo SIL validation campaign ===" << std::endl;
    std::cout << "Master seed   : " << options.seed << std::endl;
    std::cout << "Run count     : " << options.runs << std::endl;
    std::cout << "Scenario      : " << options.scenario << std::endl;

    const std::string_view brief = sim::test::scenario_brief(options.scenario);
    if (!brief.empty()) {
        std::cout << std::endl;
        std::cout << brief << std::endl;
    }

    std::cout << "Running..." << std::endl;

    const sim::monte_carlo::CampaignStats stats = sim::monte_carlo::run_campaign(options);
    sim::monte_carlo::print_summary(stats);
    std::cout << "Monte Carlo campaign completed." << std::endl;

    return 0;
}
