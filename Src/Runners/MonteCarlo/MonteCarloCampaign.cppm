/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign.cppm
Description: Monte-Carlo campaign of SIL_MONTE_CARLO : command-line options, dispersed
scenario execution and summary statistics.
Exports:
    struct CliOptions,
    struct RunMetrics,
    struct CampaignStats,
    parse_cli(),
    print_usage(),
    run_campaign(),
    print_summary()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module MonteCarloCampaign;

import std;

import Aircraft;
import PhysicsDispersion;
import Scenarios;
import TestHarness;

export namespace sim::monte_carlo {

struct CliOptions {
    std::uint64_t seed = 42ULL;
    std::uint32_t runs = 50;
    std::string   scenario = "NOMINAL-001";
    bool          help = false;
    bool          invalid = false;
};

/* Tracking metrics of one dispersed scenario run. */
struct RunMetrics {
    bool           passed = true;
    std::float64_t overshoot = 0.0;
    std::float64_t settling_time = 0.0;
    std::float64_t steady_state_error = 0.0;
    std::float64_t max_acceleration = 0.0;
};

/* Aggregated outcome of the whole Monte-Carlo campaign. */
struct CampaignStats {
    std::size_t                 total_runs = 0;
    std::size_t                 passed_runs = 0;
    std::size_t                 failed_runs = 0;
    std::vector<std::float64_t> overshoots{};
    std::vector<std::float64_t> settling_times{};
    std::vector<std::float64_t> steady_state_errors{};
    std::vector<std::float64_t> max_accelerations{};
    std::vector<std::size_t>    failed_run_indices{};
};

/* Parses the SIL_MONTE_CARLO command line into CliOptions. */
[[nodiscard]] CliOptions parse_cli(int argc, char* argv[]);

/* Prints the SIL_MONTE_CARLO usage banner. */
void print_usage(std::string_view executableName);

/*
Executes the campaign: one dispersed scenario run per iteration, accumulating
the tracking metrics and the failed-run indices.
*/
[[nodiscard]] CampaignStats run_campaign(const CliOptions& options);

/* Prints the campaign summary: run counts, pass rate and metric statistics. */
void print_summary(const CampaignStats& stats);

}
