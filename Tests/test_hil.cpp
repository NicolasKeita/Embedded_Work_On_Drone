/*
Filename: Tests/test_hil.cpp
Description: Entry point running the deterministic HIL validation suite (runner,
protocol, data-integrity and fault tests). The execution target (HIL) is selected
at runtime with --target=hil and paired with each scenario ID in the logs
([NOM-001][HIL], [FINJ-001][HIL]); the scenario logic is shared with the SIL
suite and never encodes the execution environment.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import HilTests;
import TestHarness;

namespace
{
    constexpr std::string_view kTargetPrefix = "--target=";

    struct CliOptions {
        RunTarget target = RunTarget::HIL;
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
                if (!parsed.has_value() || *parsed != sim::test::RunTarget::HIL) {
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
        std::cout << "HIL validation suite." << std::endl;
        std::cout << "Usage: test_hil [--target=hil]" << std::endl;
        std::cout << "  --target=hil   Run the scenarios under the HIL configuration (default)." << std::endl;
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
        std::cout << "Error: invalid argument. This binary runs the HIL suite; use --target=hil."
                  << std::endl;
        return 2;
    }

    sim::test::TestHarness runner{sim::test::HarnessConfig{.target = options.target}};

    sim::test::hil::run_all_hil_tests(runner);

    if (runner.passed()) {
        std::cout << "\nAll HIL tests passed." << std::endl;
        return 0;
    }
    std::cout << "\n" << runner.failure_count() << " HIL test failure(s)." << std::endl;
    return 1;
}
