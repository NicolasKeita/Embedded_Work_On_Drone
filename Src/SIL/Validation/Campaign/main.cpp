/*
Filename: Src/SIL/Validation/Campaign/main.cpp
Description: Demonstration entry point launching a 100-run Monte-Carlo validation campaign.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import FlightControlValidation;
import SilRunnerContext;

void PrintSummary(const sim::sil::validation::CampaignSummary& summary);

int main()
{
    constexpr std::uint64_t kMasterSeed = 20260825ULL;
    constexpr std::uint64_t kRunCount = 100;
    sim::sil::SilConfig config{.dt = 0.01, .duration_s = 30.0, .target = {.z = 10.0}};
    sim::sil::validation::MonteCarloRunner runner(kMasterSeed, config);

    std::cout << "=== Monte-Carlo SIL validation campaign ===" << std::endl;
    std::cout << "master seed : " << kMasterSeed << std::endl;
    std::cout << "run count   : " << kRunCount << std::endl;
    std::cout << "running..." << std::endl;

    const sim::sil::validation::CampaignSummary summary = runner.run(kRunCount);

    PrintSummary(summary);

    std::ofstream csv_out("monte_carlo_results.csv");
    runner.collector().write_csv(csv_out);
    csv_out.close();

    std::cout << "\nCSV export written to monte_carlo_results.csv ("
              << runner.collector().size() << " rows)" << std::endl;

    return 0;
}

/*
Prints the aggregate statistics of a campaign: success rate, per-domain
failure counts and mean latencies/errors. The layout mirrors the validation
contract so that a human reader can decide acceptance at a glance.
*/
void PrintSummary(const sim::sil::validation::CampaignSummary& summary)
{
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "\n--- Campaign summary ---" << std::endl;
    std::cout << "total runs        : " << summary.total_runs << std::endl;
    std::cout << "successful runs   : " << summary.successful_runs << std::endl;
    std::cout << "failed runs       : " << summary.failed_runs << std::endl;
    std::cout << "runner errors     : " << summary.runner_errors << std::endl;
    std::cout << "success rate      : " << (summary.success_rate * 100.0) << " %" << std::endl;
    std::cout << "mean detection lat: " << summary.mean_detection_latency_s << " s" << std::endl;
    std::cout << "mean response lat : " << summary.mean_response_latency_s << " s" << std::endl;
    std::cout << "mean position err : " << summary.mean_position_error_m << " m" << std::endl;
    std::cout << "mean altitude err : " << summary.mean_altitude_error_m << " m" << std::endl;

    std::cout << "\n--- Failure breakdown ---" << std::endl;
    for (std::size_t i = 0; i < summary.failure_counts.size(); ++i) {
        if (summary.failure_counts[i] == 0) {
            continue;
        }
        const auto reason = static_cast<sim::sil::validation::FailureReason>(i);
        std::string_view name = sim::sil::validation::failure_reason_name(reason);
        if (name.empty()) {
            name = "NONE";
        }
        std::cout << std::setw(22) << std::left << name << " : " << summary.failure_counts[i] << std::endl;
    }
}
