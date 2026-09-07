/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Summary.cpp
Description: Campaign summary printing and usage banner.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

namespace sim::monte_carlo {

namespace {
    /* Fills the mean, standard deviation, minimum and maximum of the sample set. */
    void compute_statistics(const std::vector<std::float64_t>& values,
                            std::float64_t&                    mean,
                            std::float64_t&                    std_dev,
                            std::float64_t&                    min_val,
                            std::float64_t&                    max_val)
    {
        if (values.empty()) {
            mean = 0.0;
            std_dev = 0.0;
            min_val = 0.0;
            max_val = 0.0;
            return;
        }
        mean = std::accumulate(values.begin(), values.end(), 0.0) / static_cast<std::float64_t>(values.size());
        std::float64_t sum_sq = 0.0;
        for (std::float64_t val : values) {
            sum_sq += (val - mean) * (val - mean);
        }
        std_dev = std::sqrt(sum_sq / static_cast<std::float64_t>(values.size()));
        min_val = *std::min_element(values.begin(), values.end());
        max_val = *std::max_element(values.begin(), values.end());
    }

    /* Prints the statistics block of one campaign metric. */
    void print_metric(std::string_view label, const std::vector<std::float64_t>& values)
    {
        if (values.empty()) { return; }
        std::float64_t mean = 0.0;
        std::float64_t std_dev = 0.0;
        std::float64_t min_val = 0.0;
        std::float64_t max_val = 0.0;
        compute_statistics(values, mean, std_dev, min_val, max_val);
        std::cout << label << ": Mean: " << mean << ", StdDev: " << std_dev
                  << ", Min: " << min_val << ", Max: " << max_val << std::endl;
        std::cout << "  3-sigma bounds: [" << (mean - 3.0 * std_dev) << ", "
                  << (mean + 3.0 * std_dev) << "]" << std::endl;
    }
}

/* Prints the campaign summary: run counts, pass rate and metric statistics. */
void print_summary(const CampaignStats& stats)
{
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\n--- Campaign Summary ---" << std::endl;
    std::cout << "Total runs     : " << stats.total_runs << std::endl;
    std::cout << "Passed runs    : " << stats.passed_runs << std::endl;
    std::cout << "Failed runs    : " << stats.failed_runs << std::endl;
    if (stats.total_runs > 0) {
        const std::float64_t pass_rate =
            (static_cast<std::float64_t>(stats.passed_runs) / stats.total_runs) * 100.0;
        std::cout << "Pass rate      : " << pass_rate << " % (" << stats.passed_runs << "/"
                  << stats.total_runs << ")" << std::endl;
    }
    std::cout << "\n--- Metric Statistics ---" << std::endl;
    print_metric("Overshoot", stats.overshoots);
    print_metric("Steady-state error", stats.steady_state_errors);
    print_metric("Settling time", stats.settling_times);
    print_metric("Max acceleration", stats.max_accelerations);
    if (!stats.failed_run_indices.empty()) {
        std::cout << "\n--- Failed Run Indices ---" << std::endl;
        for (std::size_t idx : stats.failed_run_indices) {
            std::cout << "Run " << idx << "; ";
        }
        std::cout << std::endl;
    }
}

/* Prints the SIL_MONTE_CARLO usage banner. */
void print_usage(std::string_view executableName)
{
    std::cout << "SIL Monte-Carlo: accelerated statistical batch simulation." << std::endl;
    std::cout << "Usage: " << executableName << " [--seed <n>] [--runs <n>] [--scenario <id>] [-v]" << std::endl;
    std::cout << "  --seed <n>       Master RNG seed (default 42)." << std::endl;
    std::cout << "  --runs <n>       Number of iterations (default 50)." << std::endl;
    std::cout << "  --scenario <id>  Scenario to stress-test (default: NOMINAL-001, also: 008, 009, 010)." << std::endl;
    std::cout << "  -v, --verbose     Per-run telemetry, mission brief and dispersion parameters." << std::endl;
    std::cout << "  -h, --help        Show this help." << std::endl;
}

}
