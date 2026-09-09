/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign.cppm
Description: Monte-Carlo campaign of SIL_MONTE_CARLO : command-line options, dispersed scenario execution and summary statistics.
Exports:
    CliOptions, RunInputs, RunMetrics, RunRecord, CampaignStats, parse_cli(),
    print_usage(), run_campaign(), print_summary(), write_campaign_csv(),
    write_campaign_json(), export_campaign_csv(), export_campaign_json()

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
    std::string   output_csv{};
    std::string   output_json{};
    bool          verbose = false;
    bool          help = false;
    bool          invalid = false;
};

/* Per-run perturbed input variables: deterministic seeds and physics dispersion of one iteration. */
struct RunInputs {
    std::uint32_t          run_id = 0;
    std::uint64_t          master_seed = 0;
    std::uint64_t          run_seed = 0;
    sim::PhysicsDispersion dispersion{};
};

/* Tracking metrics of one dispersed scenario run. */
struct RunMetrics {
    bool           passed = true;
    std::float64_t overshoot = 0.0;
    std::float64_t settling_time = 0.0;
    std::float64_t steady_state_error = 0.0;
    std::float64_t max_acceleration = 0.0;
};

/* Per-run record associating the perturbed input variables with the measured output metrics. */
struct RunRecord {
    RunInputs  inputs{};
    RunMetrics metrics{};
};

/* Aggregated outcome of the whole Monte-Carlo campaign. */
struct CampaignStats {
    std::uint64_t          master_seed = 0;
    std::size_t            total_runs = 0;
    std::size_t            passed_runs = 0;
    std::size_t            failed_runs = 0;
    std::vector<RunRecord> records{};
};

/* Parses the SIL_MONTE_CARLO command line into CliOptions. */
[[nodiscard]] CliOptions parse_cli(int argc, char* argv[]);

/* Prints the SIL_MONTE_CARLO usage banner. */
void print_usage(std::string_view executableName);

/* Executes the campaign: one dispersed scenario run per iteration, accumulating the per-run records. */
[[nodiscard]] CampaignStats run_campaign(const CliOptions& options);

/* Prints the campaign summary: run counts, pass rate and metric statistics. */
void print_summary(const CampaignStats& stats);

/* Writes the full campaign report as CSV (header + one row per run) into the given stream. */
void write_campaign_csv(std::ostream& out, const CampaignStats& stats);

/* Writes the full campaign report as a JSON array (one object per run) into the given stream. */
void write_campaign_json(std::ostream& out, const CampaignStats& stats);

/* Writes the campaign CSV report to <path>; false with a diagnostic when the file cannot be opened. */
[[nodiscard]] bool export_campaign_csv(const CampaignStats& stats, std::string_view path);

/* Writes the campaign JSON report to <path>; false with a diagnostic when the file cannot be opened. */
[[nodiscard]] bool export_campaign_json(const CampaignStats& stats, std::string_view path);

}

namespace sim::monte_carlo {

/* Prints the perturbed input variables (RunInputs) generated for one run. */
void print_run_inputs(const RunInputs& inputs, std::uint32_t run_count);

/* Applies one recognized campaign option; returns false when parsing must stop. */
bool apply_option(CliOptions& options, int argc, char* argv[], int& index, std::string_view argument);

}
