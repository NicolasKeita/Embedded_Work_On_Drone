/*
Filename: Src/SIL/Validation/FlightControlValidation.cppm
Description: Monte-Carlo validation subsystem interface : scenario generator, result collector and
runner orchestration (types are re-exported from ValidationTypes).
Exports:
    class ScenarioGenerator,
    class ResultCollector,
    class MonteCarloRunner,
    sample_fault_window(),
    sample_fault_parameters(),
    sample_environment(),
    sample_initial_conditions()
    (re-exports ValidationTypes)

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FlightControlValidation;

import std;

import SilRunner;
import SilTypes;
import ValidationTypes;

export import ValidationTypes;

export namespace sim::sil::validation {

class ScenarioGenerator {
public:
    explicit ScenarioGenerator(std::uint64_t master_seed);
    /*
    Generates one run scenario; when fault_template has a value the sampled fault
    family is overridden with it (the RNG stream is unchanged, so the campaign
    stays reproducible for a given master seed).
    */
    [[nodiscard]] Scenario generate_scenario(std::uint64_t run_id,
                                             std::optional<FailureMode> fault_template = std::nullopt);
    [[nodiscard]] std::uint64_t master_seed() const noexcept;
private:
    static std::uint64_t mix_seed(std::uint64_t master_seed, std::uint64_t run_id);
    std::uint64_t master_seed_;
};

class ResultCollector {
public:
    ResultCollector() = default;
    void record(SimulationResult result);
    void reserve(std::size_t count);
    [[nodiscard]] std::span<const SimulationResult> results() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] CampaignSummary summarize() const;
    void write_csv(std::ostream& out) const;
    void clear() noexcept;
private:
    std::vector<SimulationResult> results_;
};

class MonteCarloRunner {
public:
    MonteCarloRunner(std::uint64_t master_seed, SilConfig config = {});
    /*
    Restricts every generated run to one fault family (the scenario template of
    the campaign, e.g. FAULT_INJECTOR-002 forces FC_COMMUNICATION_LOSS); a nullopt
    template keeps the fully random fault dispersion. FailureMode::NONE produces
    nominal-only runs.
    */
    void set_fault_template(std::optional<FailureMode> fault_template) noexcept;
    [[nodiscard]] CampaignSummary run(std::uint64_t run_count);
    [[nodiscard]] const ResultCollector& collector() const noexcept;
    [[nodiscard]] ResultCollector& collector() noexcept;
    [[nodiscard]] std::uint64_t master_seed() const noexcept;
private:
    static SimulationResult classify(const Scenario& scenario,
                                      const sim::sil::SimulationResult& sil_result);
    void execute_run(std::uint64_t run_id);
    ScenarioGenerator          generator_;
    SilConfig                  config_;
    ResultCollector            collector_;
    std::optional<FailureMode> fault_template_{};
};

void sample_fault_window(Scenario& scenario, std::mt19937_64& generator,
                         std::optional<FailureMode> fault_template = std::nullopt);
void sample_fault_parameters(Scenario& scenario, std::mt19937_64& generator);
void sample_environment(Scenario& scenario, std::mt19937_64& generator);
void sample_initial_conditions(Scenario& scenario, std::mt19937_64& generator);

}

namespace sim::sil::validation {
void write_csv_row(std::ostream& out, const SimulationResult& result);
}
