/*
Filename: Tests/test_sil.cpp
Description: Entry point running the deterministic SIL validation suite (nominal,
fault-injection, observability and telemetry scenarios). The execution target
(SIL) is selected at runtime with --target=sil and paired with each scenario ID
in the logs ([NOM-001][SIL], [FINJ-001][SIL]); the scenario logic is shared with
the HIL suite and never encodes the execution environment.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import SilScenarios;
import TestHarness;

namespace
{
    constexpr std::string_view kTargetPrefix = "--target=";

    struct CliOptions {
        RunTarget target = RunTarget::SIL;
        bool      help = false;
        bool      invalid = false;
    };

    CliOptions parse_cli(int argc, char* argv[])
    {
        CliOptions options;
        for (int index = 1; index < argc; ++index) {
            const char*       raw = argv[index] != nullptr ? argv[index] : "";
            const std::string_view argument{raw};

            if (argument == "-h" || argument == "--help") {
                options.help = true;
                return options;
            }
            if (argument.starts_with(kTargetPrefix)) {
                const std::string_view                 name = argument.substr(kTargetPrefix.size());
                const std::optional<sim::test::RunTarget> parsed = sim::test::parse_run_target(name);
                if (!parsed.has_value() || *parsed != sim::test::RunTarget::SIL) {
                    options.invalid = true;
                    return options;
                }
                options.target = *parsed;
                continue;
            }
            options.invalid = true;
            return options;
        }
        return options;
    }

    int print_usage()
    {
        std::cout << "SIL validation suite." << std::endl;
        std::cout << "Usage: test_sil [--target=sil]" << std::endl;
        std::cout << "  --target=sil   Run the scenarios under the SIL configuration (default)." << std::endl;
        std::cout << "  -h, --help     Show this help and exit." << std::endl;
        return 0;
    }
}

int main(int argc, char* argv[])
{
    const CliOptions options = parse_cli(argc, argv);

    if (options.help) {
        return print_usage();
    }
    if (options.invalid) {
        std::cout << "Error: invalid argument. This binary runs the SIL suite; use --target=sil."
                  << std::endl;
        return 2;
    }

    sim::test::TestHarness runner{sim::test::HarnessConfig{.target = options.target}};

    sim::test::sil::run_all_sil_scenarios(runner);

    if (runner.passed()) {
        std::cout << "\nAll SIL scenarios passed." << std::endl;
        return 0;
    }
    std::cout << "\n" << runner.failure_count() << " SIL scenario failure(s)." << std::endl;
    return 1;
}
