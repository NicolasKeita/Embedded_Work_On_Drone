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
import Stm32Discovery;

export namespace sim::hil {

struct HilCliOptions {
    std::string                   scenario_id{"NOMINAL-001"};
    std::string                   interface_name{"auto"};
    std::optional<std::float64_t> duration{};
    std::optional<std::float64_t> telemetry_period{};
    std::optional<std::uint64_t>  seed{};
    std::optional<std::float64_t> noise{};
    std::string                   deadline_name{};
    bool                          selftest = false;
    bool                          list_only = false;
    bool                          help = false;
    bool                          invalid = false;
};

/* Parses the HIL_RUNNER command line into HilCliOptions. */
[[nodiscard]] HilCliOptions parse_hil_cli(int argc, char** argv);

/* Prints the HIL_RUNNER usage. */
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

namespace sim::hil {

/* Applies a string-valued option to the requested field. */
[[nodiscard]] std::expected<void, std::string> apply_string_option(std::string& field, int argc,
                                                                   char** argv, int& i,
                                                                   std::string_view label);

/* Applies a float-valued option to the requested field. */
[[nodiscard]] std::expected<void, std::string> apply_float_argument(std::optional<std::float64_t>& field,
                                                                    int argc, char** argv, int& i,
                                                                    std::string_view label);

/* Applies the --seed option to the campaign options. */
[[nodiscard]] std::expected<void, std::string> apply_seed_argument(HilCliOptions& options, int argc,
                                                                   char** argv, int& i);

/* Selects the safe loopback fallback associated with a failed hardware discovery. */
void apply_discovery_fallback(HilConfig& config, Stm32DiscoveryStatus status);

/* Resolves loopback/auto/explicit interface selection into the configuration. */
void resolve_hil_interface(HilConfig& config, std::string_view requested,
                           const Stm32DiscoveryResult& discovered);

}
