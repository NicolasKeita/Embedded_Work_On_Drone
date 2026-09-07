/*
Filename: Src/Runners/SilRunnerMain.cpp
Description: Entry point of SIL_RUNNER : deterministic software-in-the-loop test runner
executing at maximum CPU speed. Selects one scenario with --scenario (engine-level
suite: NOMINAL-001, FAULT_INJECTOR-001..004; physics
and autonomous catalog: NOMINAL-002..010) or sweeps every
deterministic scenario when none is given.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import Aircraft;
import Scenarios;
import SilScenarios;
import TestHarness;

namespace
{
    struct CliOptions {
        std::optional<std::string> scenario;
        bool                       verbose = false;
        bool                       help = false;
        bool                       invalid = false;
    };

    /*
    Parses the SIL_RUNNER command line: --scenario <id> (or --scenario=<id>),
    --verbose/-v and -h/--help. Any other argument marks the options invalid.
    */
    CliOptions parse_cli(int argc, char* argv[])
    {
        CliOptions options;
        constexpr std::string_view kScenarioPrefix = "--scenario=";

        for (int index = 1; index < argc; ++index) {
            const char*             raw = argv[index] != nullptr ? argv[index] : "";
            const std::string_view  argument{raw};

            if (argument == "-h" || argument == "--help") {
                options.help = true;
                return options;
            }
            if (argument == "-v" || argument == "--verbose") {
                options.verbose = true;
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

    /* Prints the usage banner and the full deterministic scenario catalog. */
    void print_usage(std::string_view executableName)
    {
        std::cout << "SIL runner: deterministic software-in-the-loop execution at maximum CPU speed." << std::endl;
        std::cout << "Usage: " << executableName << " [--scenario <id>] [-v | --verbose]" << std::endl;
        std::cout << "  --scenario <id>  Run one scenario (all scenarios when omitted)." << std::endl;
        std::cout << "  -v, --verbose    Per-step telemetry logging." << std::endl;
        std::cout << "  -h, --help       Show this help." << std::endl;
        std::cout << std::endl;
        std::cout << "SIL engine scenarios:" << std::endl;
        for (const sim::test::sil::SilScenarioEntry& entry : sim::test::sil::sil_scenarios()) {
            std::cout << "  " << entry.id << " : " << entry.description << std::endl;
        }
        std::cout << "Physics and autonomous scenarios:" << std::endl;
        for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
            std::cout << "  " << entry.id << " : " << entry.description << std::endl;
        }
    }
}

/*
Entry point: a single harness is shared by every executed scenario so the
failure counter accumulates over the whole run; the exit code mirrors the
verdict (0 = all passed, 1 = at least one failure, 2 = command-line error).
*/
int main(int argc, char* argv[])
{
    const std::string_view executableName = (argc > 0 && argv[0] != nullptr) ? argv[0] : "SIL_RUNNER";
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

    const std::size_t    logIntervalSteps = options.verbose ? std::size_t{1} : std::size_t{100};
    sim::test::HarnessConfig harnessConfig{
        .dt = 0.01, .log_interval_steps = logIntervalSteps, .target = sim::test::RunTarget::SIL};
    sim::test::TestHarness runner{harnessConfig};

    if (options.scenario.has_value()) {
        const std::string& scenarioId = *options.scenario;
        if (sim::test::sil::find_sil_scenario(scenarioId) != nullptr) {
            sim::test::sil::run_sil_scenario(scenarioId, runner);
        }
        else if (sim::test::ScenarioCatalog::find(scenarioId) != nullptr) {
            const Aircraft reference;
            sim::test::ScenarioCatalog::find(scenarioId)->run(runner, reference.hover_rpm(), {});
        }
        else {
            std::cout << "Error: unknown scenario \"" << scenarioId << "\"." << std::endl;
            std::cout << std::endl;
            print_usage(executableName);
            return 2;
        }
    }
    else {
        std::cout << "=== SIL_RUNNER deterministic sweep (software-in-the-loop, max CPU speed) ===" << std::endl;
        sim::test::sil::run_all_sil_scenarios(runner);

        std::cout << "\n=== Physics and autonomous functional scenarios ===" << std::endl;
        const Aircraft reference;
        for (const sim::test::ScenarioEntry& entry : sim::test::ScenarioCatalog::all()) {
            entry.run(runner, reference.hover_rpm(), {});
        }
    }

    if (runner.passed()) {
        std::cout << "\n>>> SIL_RUNNER: all executed scenarios passed." << std::endl;
        return 0;
    }
    std::cout << "\n>>> SIL_RUNNER: " << runner.failure_count() << " verification(s) failed." << std::endl;
    return 1;
}