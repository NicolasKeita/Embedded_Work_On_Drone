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

import FlightControlValidation;
import SilFaultScenario;
import SilRunnerContext;

namespace
{
    constexpr std::string_view kScenarioPrefix = "--scenario=";

    struct CliOptions {
        std::uint64_t seed = 20260825ULL;
        std::uint64_t runs = 100;
        std::string   scenario{};
        bool          help = false;
        bool          invalid = false;
    };

    /*
    Exception-free unsigned conversion; returns std::nullopt on malformed input
    instead of throwing.
    */
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

    void apply_unsigned(std::uint64_t& field, std::string_view raw, std::string_view label, CliOptions& options)
    {
        const std::optional<std::uint64_t> value = parse_unsigned(raw);
        if (!value.has_value()) {
            std::cerr << "Error: invalid value for " << label << ": " << raw << std::endl;
            options.invalid = true;
            return;
        }
        field = *value;
    }

    /* Parses the SIL_MONTE_CARLO command line (--seed, --runs, --scenario, -h). */
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
                apply_unsigned(options.seed, argv[index] != nullptr ? argv[index] : "", "--seed", options);
                continue;
            }
            if (argument == "--runs") {
                if (index + 1 >= argc) {
                    std::cerr << "Error: missing value for --runs." << std::endl;
                    options.invalid = true;
                    return options;
                }
                ++index;
                apply_unsigned(options.runs, argv[index] != nullptr ? argv[index] : "", "--runs", options);
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

    /*
    Maps a scenario template ID onto the fault family it restricts the campaign
    to; an empty ID keeps the fully random dispersion. Returns false for an
    unknown template.
    */
    bool parse_fault_template(const std::string& scenarioId, std::optional<sim::sil::FaultType>& faultTemplate)
    {
        faultTemplate = std::nullopt;
        if (scenarioId.empty()) {
            return true;
        }
        if (scenarioId == "NOMINAL-001") {
            faultTemplate = sim::sil::FaultType::None;
            return true;
        }
        if (scenarioId == "FAULT_INJECTOR-001") {
            faultTemplate = sim::sil::FaultType::FC1Failure;
            return true;
        }
        if (scenarioId == "FAULT_INJECTOR-002") {
            faultTemplate = sim::sil::FaultType::CommunicationLoss;
            return true;
        }
        if (scenarioId == "FAULT_INJECTOR-003") {
            faultTemplate = sim::sil::FaultType::SensorFault;
            return true;
        }
        if (scenarioId == "FAULT_INJECTOR-004") {
            faultTemplate = sim::sil::FaultType::ActuatorDegradation;
            return true;
        }
        return false;
    }

    /* Prints the campaign usage. */
    void print_usage(std::string_view executableName)
    {
        std::cout << "SIL Monte-Carlo: accelerated statistical batch simulation." << std::endl;
        std::cout << "Usage: " << executableName << " [--seed <n>] [--runs <n>] [--scenario <template>]" << std::endl;
        std::cout << "  --seed <n>       Master RNG seed (default 20260825)." << std::endl;
        std::cout << "  --runs <n>       Number of iterations (default 100)." << std::endl;
        std::cout << "  --scenario <id>  Optional scenario template filter:" << std::endl;
        std::cout << "                     NOMINAL-001          nominal-only runs" << std::endl;
        std::cout << "                     FAULT_INJECTOR-001   FC1 failure runs" << std::endl;
        std::cout << "                     FAULT_INJECTOR-002   communication loss runs" << std::endl;
        std::cout << "                     FAULT_INJECTOR-003   sensor fault runs" << std::endl;
        std::cout << "                     FAULT_INJECTOR-004   actuator degradation runs" << std::endl;
        std::cout << "  -h, --help       Show this help." << std::endl;
    }

    /* Prints the aggregate statistics of a campaign: success rate, per-domain
    failure counts and mean latencies/errors. The layout mirrors the validation
    contract so that a human reader can decide acceptance at a glance. */
    void print_summary(const sim::sil::validation::CampaignSummary& summary)
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
}

/*
Entry point: N deterministic SIL runs sharing one master seed, summarised to
the terminal and exported as CSV (monte_carlo_results.csv).
*/
int main(int argc, char* argv[])
{
    const std::string_view executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "SIL_MONTE_CARLO";
    const CliOptions       options = parse_cli(argc, argv);

    if (options.help) {
        print_usage(executableName);
        return 0;
    }
    if (options.invalid) {
        std::cout << std::endl;
        print_usage(executableName);
        return 2;
    }

    std::optional<sim::sil::FaultType> faultTemplate{};
    if (!parse_fault_template(options.scenario, faultTemplate)) {
        std::cout << "Error: unknown scenario template \"" << options.scenario << "\"." << std::endl;
        std::cout << std::endl;
        print_usage(executableName);
        return 2;
    }

    sim::sil::SilConfig                    config{.dt = 0.01, .duration_s = 30.0, .target = {.z = 10.0}};
    sim::sil::validation::MonteCarloRunner runner(options.seed, config);
    runner.set_fault_template(faultTemplate);

    std::cout << "=== Monte-Carlo SIL validation campaign ===" << std::endl;
    std::cout << "master seed : " << options.seed << std::endl;
    std::cout << "run count   : " << options.runs << std::endl;
    if (faultTemplate.has_value()) {
        std::cout << "template    : " << options.scenario << std::endl;
    }
    std::cout << "running..." << std::endl;

    const sim::sil::validation::CampaignSummary summary = runner.run(options.runs);

    print_summary(summary);

    std::ofstream csv_out("monte_carlo_results.csv");
    runner.collector().write_csv(csv_out);
    csv_out.close();

    std::cout << "\nCSV export written to monte_carlo_results.csv ("
              << runner.collector().size() << " rows)" << std::endl;

    return 0;
}