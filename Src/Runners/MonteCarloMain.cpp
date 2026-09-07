/*
Filename: Src/Runners/MonteCarloMain.cpp
Description: Entry point of SIL_MONTE_CARLO : accelerated statistical batch simulation
over the SIL engine. --seed selects the master RNG seed, --runs the iteration count
and --scenario optionally restricts the campaign to one scenario template
(NOMINAL-001 or FAULT_INJECTOR-001..004). Results are exported as CSV.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import FlightController;
import MissionRunner;
import PhysicsDispersion;
import Scenarios;
import TestHarness;
import FlightScenarios;

namespace
{
    constexpr std::string_view kScenarioPrefix = "--scenario=";

    struct CliOptions {
        std::uint64_t seed = 42ULL;
        std::uint32_t runs = 50;
        std::string   scenario = "NOMINAL-001";
        bool          help = false;
        bool          invalid = false;
    };

    std::optional<std::uint64_t> parse_unsigned(std::string_view text)
    {
        std::uint64_t value = 0;
        const std::from_chars_result result =
            std::from_chars(text.data(), text.data() + text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) {
            return std::nullopt;
        }
        return value;
    }

    void apply_unsigned_uint64(std::uint64_t& field, std::string_view raw, std::string_view label, CliOptions& options)
    {
        const std::optional<std::uint64_t> value = parse_unsigned(raw);
        if (!value.has_value()) {
            std::cerr << "Error: invalid value for " << label << ": " << raw << std::endl;
            options.invalid = true;
            return;
        }
        field = *value;
    }

    void apply_unsigned_uint32(std::uint32_t& field, std::string_view raw, std::string_view label, CliOptions& options)
    {
        const std::optional<std::uint64_t> value = parse_unsigned(raw);
        if (!value.has_value()) {
            std::cerr << "Error: invalid value for " << label << ": " << raw << std::endl;
            options.invalid = true;
            return;
        }
        field = static_cast<std::uint32_t>(*value);
    }

    CliOptions parse_cli(int argc, char* argv[])
    {
        CliOptions options;

        for (int index = 1; index < argc; ++index) {
            const char*            raw = argv[index] != nullptr ? argv[index] : "";
            const std::string_view argument{raw};

            if (argument == "-h" || argument == "--help") {
                options.help = true;
                return options;
            }
            if (argument == "--seed") {
                if (index + 1 >= argc) {
                    std::cerr << "Error: missing value for --seed." << std::endl;
                    options.invalid = true;
                    return options;
                }
                ++index;
                apply_unsigned_uint64(options.seed, argv[index] != nullptr ? argv[index] : "", "--seed", options);
                continue;
            }
            if (argument == "--runs") {
                if (index + 1 >= argc) {
                    std::cerr << "Error: missing value for --runs." << std::endl;
                    options.invalid = true;
                    return options;
                }
                ++index;
                apply_unsigned_uint32(options.runs, argv[index] != nullptr ? argv[index] : "", "--runs", options);
                continue;
            }
            if (argument == "--scenario") {
                if (index + 1 >= argc) {
                    std::cerr << "Error: missing value for --scenario." << std::endl;
                    options.invalid = true;
                    return options;
                }
                ++index;
                options.scenario = argv[index] != nullptr ? argv[index] : "";
                continue;
            }
            if (argument.starts_with(kScenarioPrefix)) {
                options.scenario = std::string{argument.substr(kScenarioPrefix.size())};
                continue;
            }
            std::cerr << "Error: unknown argument \"" << argument << "\"." << std::endl;
            options.invalid = true;
            return options;
        }
        return options;
    }

    void print_usage(std::string_view executableName)
    {
        std::cout << "SIL Monte-Carlo: accelerated statistical batch simulation." << std::endl;
        std::cout << "Usage: " << executableName << " [--seed <n>] [--runs <n>] [--scenario <template>]" << std::endl;
        std::cout << "  --seed <n>       Master RNG seed (default 42)." << std::endl;
        std::cout << "  --runs <n>       Number of iterations (default 50)." << std::endl;
        std::cout << "  --scenario <id>  Scenario to stress-test (default: NOMINAL-001)" << std::endl;
        std::cout << "                     Examples: NOMINAL-001, NOMINAL-008, NOMINAL-009, NOMINAL-010" << std::endl;
        std::cout << "  -h, --help       Show this help." << std::endl;
    }

    struct RunMetrics {
        bool passed = true;
        std::float64_t overshoot = 0.0;
        std::float64_t settling_time = 0.0;
        std::float64_t steady_state_error = 0.0;
        std::float64_t max_acceleration = 0.0;
    };

    struct CampaignStats {
        std::size_t total_runs = 0;
        std::size_t passed_runs = 0;
        std::size_t failed_runs = 0;
        std::vector<std::float64_t> overshoots;
        std::vector<std::float64_t> settling_times;
        std::vector<std::float64_t> steady_state_errors;
        std::vector<std::float64_t> max_accelerations;
        std::vector<std::size_t> failed_run_indices;
    };

    void compute_statistics(const std::vector<std::float64_t>& values,
                           std::float64_t& mean, std::float64_t& std_dev, 
                           std::float64_t& min_val, std::float64_t& max_val) {
        if (values.empty()) {
            mean = std_dev = min_val = max_val = 0.0;
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

    void print_summary(const CampaignStats& stats)
    {
        std::cout << std::fixed << std::setprecision(4);
        
        std::cout << "\n--- Campaign Summary ---" << std::endl;
        std::cout << "Total runs     : " << stats.total_runs << std::endl;
        std::cout << "Passed runs    : " << stats.passed_runs << std::endl;
        std::cout << "Failed runs    : " << stats.failed_runs << std::endl;
        
        if (stats.total_runs > 0) {
            const std::float64_t pass_rate = (static_cast<std::float64_t>(stats.passed_runs) / stats.total_runs) * 100.0;
            std::cout << "Pass rate      : " << pass_rate << " % (" << stats.passed_runs << "/" << stats.total_runs << ")" << std::endl;
        }
        
        std::cout << "\n--- Metric Statistics ---" << std::endl;
        
        if (!stats.overshoots.empty()) {
            std::float64_t mean, std_dev, min_val, max_val;
            compute_statistics(stats.overshoots, mean, std_dev, min_val, max_val);
            std::cout << "Overshoot:" << std::endl;
            std::cout << "  Mean: " << mean << ", StdDev: " << std_dev << 
                      ", Min: " << min_val << ", Max: " << max_val << std::endl;
            std::cout << "  3-sigma bounds: [" << (mean - 3.0 * std_dev) << ", " << (mean + 3.0 * std_dev) << "]" << std::endl;
        }
        
        if (!stats.steady_state_errors.empty()) {
            std::float64_t mean, std_dev, min_val, max_val;
            compute_statistics(stats.steady_state_errors, mean, std_dev, min_val, max_val);
            std::cout << "Steady-State Error:" << std::endl;
            std::cout << "  Mean: " << mean << ", StdDev: " << std_dev << 
                      ", Min: " << min_val << ", Max: " << max_val << std::endl;
            std::cout << "  3-sigma bounds: [" << (mean - 3.0 * std_dev) << ", " << (mean + 3.0 * std_dev) << "]" << std::endl;
        }

        if (!stats.settling_times.empty()) {
            std::float64_t mean;
            std::float64_t std_dev;
            std::float64_t min_val;
            std::float64_t max_val;
            compute_statistics(stats.settling_times, mean, std_dev, min_val, max_val);
            std::cout << "Settling time: Mean: " << mean << ", StdDev: " << std_dev
                      << ", Min: " << min_val << ", Max: " << max_val << std::endl;
        }

        if (!stats.max_accelerations.empty()) {
            std::float64_t mean;
            std::float64_t std_dev;
            std::float64_t min_val;
            std::float64_t max_val;
            compute_statistics(stats.max_accelerations, mean, std_dev, min_val, max_val);
            std::cout << "Max acceleration: Mean: " << mean << ", StdDev: " << std_dev
                      << ", Min: " << min_val << ", Max: " << max_val << std::endl;
        }
        
        if (!stats.failed_run_indices.empty()) {
            std::cout << "\n--- Failed Run Indices ---" << std::endl;
            for (std::size_t idx : stats.failed_run_indices) {
                std::cout << "Run " << idx << "; ";
            }
            std::cout << std::endl;
        }
    }

    RunMetrics execute_scenario_run(const std::string& scenario_id,
                                    const sim::PhysicsDispersion& dispersion)
    {
        RunMetrics metrics{};
        const sim::test::ScenarioEntry* scenario_entry = sim::test::ScenarioCatalog::find(scenario_id);
        if (scenario_entry == nullptr) {
            std::cerr << "Error: Unknown scenario " << scenario_id << std::endl;
            metrics.passed = false;
            return metrics;
        }

        sim::test::TestHarness runner;
        const Aircraft reference{dispersion};
        scenario_entry->run(runner, reference.hover_rpm(), dispersion);
        metrics.passed = runner.passed();
        metrics.overshoot = runner.overshoot();
        metrics.settling_time = runner.settling_time();
        metrics.steady_state_error = runner.steady_state_error();
        metrics.max_acceleration = runner.max_acceleration();
        return metrics;
    }
}

int main(int argc, char* argv[])
{
    const std::string_view executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "SIL_MONTE_CARLO";
    const CliOptions options = parse_cli(argc, argv);

    if (options.help) {
        print_usage(executableName);
        return 0;
    }
    if (options.invalid) {
        std::cout << std::endl;
        print_usage(executableName);
        return 2;
    }

    std::cout << "=== Monte-Carlo SIL validation campaign ===" << std::endl;
    std::cout << "Master seed   : " << options.seed << std::endl;
    std::cout << "Run count     : " << options.runs << std::endl;
    std::cout << "Scenario      : " << options.scenario << std::endl;
    std::cout << "Running..." << std::endl;

    sim::DispersionGenerator dispersion_generator(options.seed);
    CampaignStats campaign_stats;
    campaign_stats.total_runs = options.runs;

    for (std::uint32_t run_index = 0; run_index < options.runs; ++run_index) {
        sim::PhysicsDispersion dispersion = dispersion_generator.generate_run_dispersion(run_index);
        RunMetrics run_metrics = execute_scenario_run(options.scenario, dispersion);
        
        if (run_metrics.passed) {
            campaign_stats.passed_runs++;
        } else {
            campaign_stats.failed_runs++;
            campaign_stats.failed_run_indices.push_back(run_index);
        }
        
        campaign_stats.overshoots.push_back(run_metrics.overshoot);
        campaign_stats.settling_times.push_back(run_metrics.settling_time);
        campaign_stats.steady_state_errors.push_back(run_metrics.steady_state_error);
        campaign_stats.max_accelerations.push_back(run_metrics.max_acceleration);
        
        if ((run_index + 1) % 10 == 0 || run_index + 1 == options.runs) {
            std::cout << "Completed run " << (run_index + 1) << "/" << options.runs << std::endl;
        }
    }

    print_summary(campaign_stats);
    std::cout << "Monte Carlo campaign completed." << std::endl;

    return 0;
}