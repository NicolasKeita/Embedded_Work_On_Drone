/*
Filename: Src/Runners/Sil/SilRunnerCli.cppm
Description: Command-line interface of SIL_RUNNER : option parsing and usage printing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunnerCli;

import std;

import Scenarios;
import SilScenarios;

export namespace sim::test::sil {

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
[[nodiscard]] CliOptions parse_cli(int argc, char* argv[]);

/* Prints the usage banner and the full deterministic scenario catalog. */
void print_usage(std::string_view executableName);

}