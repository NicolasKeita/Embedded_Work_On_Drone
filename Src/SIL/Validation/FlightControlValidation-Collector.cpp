/*
Filename: Src/SIL/Validation/FlightControlValidation-Collector.cpp
Description: Result storage, statistical aggregation and typed CSV export of Monte-Carlo campaign outcomes.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FlightControlValidation;

import std;

import SilTypes;

namespace sim::sil::validation {

void ResultCollector::record(SimulationResult result)
{
    results_.push_back(std::move(result));
}

void ResultCollector::reserve(std::size_t count)
{
    results_.reserve(count);
}

std::span<const SimulationResult> ResultCollector::results() const noexcept
{
    return results_;
}

std::size_t ResultCollector::size() const noexcept
{
    return results_.size();
}

void ResultCollector::clear() noexcept
{
    results_.clear();
    results_.shrink_to_fit();
}

struct Accumulators {
    std::float64_t detection_sum = 0.0;
    std::float64_t response_sum = 0.0;
    std::float64_t position_sum = 0.0;
    std::float64_t altitude_sum = 0.0;
    std::uint64_t detection_count = 0;
    std::uint64_t response_count = 0;
};

/*
Accumulates one run into the summary counters and running sums. Latency sums
are only fed when the run produced a finite latency; runner errors skip them.
*/
static void accumulate_run(const SimulationResult& result,
                           CampaignSummary&        summary,
                           Accumulators&           acc)
{
    if (result.failure_reason == FailureReason::RunnerError) {
        ++summary.runner_errors;
        return;
    }

    if (result.verdict) {
        ++summary.successful_runs;
    } else {
        ++summary.failed_runs;
    }

    const std::uint32_t reason_id = static_cast<std::uint32_t>(result.failure_reason);
    if (reason_id < summary.failure_counts.size()) {
        ++summary.failure_counts[reason_id];
    }
    if (result.sil_result.detection_latency >= 0.0) {
        acc.detection_sum += result.sil_result.detection_latency;
        ++acc.detection_count;
    }
    if (result.sil_result.response_latency >= 0.0) {
        acc.response_sum += result.sil_result.response_latency;
        ++acc.response_count;
    }
    acc.position_sum += result.position_error_m;
    acc.altitude_sum += result.altitude_error_m;
}

/*
Single-pass aggregation of every stored run: success rate, per-domain failure
counts and mean latencies/errors. The accumulation is delegated to
accumulate_run so that summarize stays a thin fold over the result buffer.
*/
CampaignSummary ResultCollector::summarize() const
{
    CampaignSummary summary{.total_runs = results_.size()};
    Accumulators    acc{};

    for (const SimulationResult& result : results_) {
        accumulate_run(result, summary, acc);
    }

    if (summary.total_runs > 0) {
        summary.success_rate =
            static_cast<std::float64_t>(summary.successful_runs) / static_cast<std::float64_t>(summary.total_runs);
        summary.mean_position_error_m = acc.position_sum / static_cast<std::float64_t>(summary.total_runs);
        summary.mean_altitude_error_m = acc.altitude_sum / static_cast<std::float64_t>(summary.total_runs);
    }
    if (acc.detection_count > 0) {
        summary.mean_detection_latency_s = acc.detection_sum / static_cast<std::float64_t>(acc.detection_count);
    }
    if (acc.response_count > 0) {
        summary.mean_response_latency_s = acc.response_sum / static_cast<std::float64_t>(acc.response_count);
    }

    return summary;
}

}
