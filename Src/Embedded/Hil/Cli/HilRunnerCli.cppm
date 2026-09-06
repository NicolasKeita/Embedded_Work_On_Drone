/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli.cppm
Description: Command-line interface of hil_runner : option parsing, usage/scenario
listing and the HilConfig assembly from the selected scenario plus the overrides.
Exports:
    struct HilCliOptions,
    parse_hil_cli(),
    print_hil_usage(),
    list_hil_scenarios(),
    hil_config_from_options(),
    hil_fault_from_options()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunnerCli;

import std;

import HilConfig;
import HilScenarios;
import SilFaultScenario;

export namespace sim::hil {

struct HilCliOptions {
    std::string                   scenario_id{"NOM-001_StationKeeping"};
    std::optional<std::float64_t> duration{};
    std::optional<std::float64_t> telemetry_period{};
    std::optional<std::uint64_t>  seed{};
    std::optional<std::float64_t> noise{};
    std::string                   clock_name{};
    std::string                   deadline_name{};
    bool                          no_realtime = false;
    bool                          list_only = false;
    bool                          help = false;
};

/* Parses the hil_runner command line into HilCliOptions. */
[[nodiscard]] HilCliOptions parse_hil_cli(int argc, char** argv);

/* Prints the hil_runner usage. */
void print_hil_usage(std::string_view name);

/* Lists the available HIL scenarios. */
void list_hil_scenarios();

/*
Builds the HilConfig of the selected scenario with the command-line overrides applied.
*/
[[nodiscard]] HilConfig hil_config_from_options(const HilCliOptions& options);

/* Returns the fault scenario bound to the selected HIL scenario record (empty when nominal). */
[[nodiscard]] sim::sil::FaultScenario hil_fault_from_options(const HilCliOptions& options);

}
