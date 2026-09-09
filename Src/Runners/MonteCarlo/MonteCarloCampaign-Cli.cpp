/*
Filename: Src/Runners/MonteCarlo/MonteCarloCampaign-Cli.cpp
Description: Entry of the Monte-Carlo campaign command line parsing into CliOptions.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module MonteCarloCampaign;

import std;

namespace sim::monte_carlo {

/* Parses the SIL_MONTE_CARLO command line into CliOptions. */
CliOptions parse_cli(int argc, char* argv[])
{
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument = argv[index] != nullptr ? argv[index] : "";
        if (argument == "-h" || argument == "--help") {
            options.help = true;
            return options;
        }
        if (!apply_option(options, argc, argv, index, argument)) {
            return options;
        }
    }
    return options;
}

}
