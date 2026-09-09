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

namespace
{
    /* Writes the requested CSV / JSON campaign reports and prints the outcome lines. */
    void export_reports(const sim::monte_carlo::CliOptions&    options,
                        const sim::monte_carlo::CampaignStats& stats)
    {
        if (!options.output_csv.empty() && sim::monte_carlo::export_campaign_csv(stats, options.output_csv)) {
            std::cout << "CSV report written to " << options.output_csv << std::endl;
        }
        if (!options.output_json.empty() && sim::monte_carlo::export_campaign_json(stats, options.output_json)) {
            std::cout << "JSON report written to " << options.output_json << std::endl;
        }
    }
}

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
    if (!options.output_csv.empty()) {
        std::cout << "CSV report    : " << options.output_csv << std::endl;
    }
    if (!options.output_json.empty()) {
        std::cout << "JSON report   : " << options.output_json << std::endl;
    }
    std::cout << std::endl;
    std::cout << "Running..." << std::endl;

    const sim::monte_carlo::CampaignStats stats = sim::monte_carlo::run_campaign(options);
    sim::monte_carlo::print_summary(stats);
    export_reports(options, stats);
    std::cout << "Monte Carlo campaign completed." << std::endl;

    return 0;
}
