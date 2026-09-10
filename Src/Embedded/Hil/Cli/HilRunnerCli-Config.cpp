/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Config.cpp
Description: HilConfig assembly of hil_runner : selected scenario record plus the
command-line overrides (duration, cadence, seed, noise and deadline policy).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunnerCli;

import std;

import HilConfig;
import HilScenarios;
import SilFaultScenario;
import Stm32Discovery;

namespace sim::hil {

/* Applies the numeric command-line overrides to a configuration. */
void apply_numeric_overrides(HilConfig& config, const HilCliOptions& options)
{
    if (options.duration) {
        config.duration_s = *options.duration;
    }
    if (options.telemetry_period) {
        config.report_period_s = *options.telemetry_period;
    }
    if (options.seed) {
        config.seed = *options.seed;
    }
    if (options.noise) {
        config.sensor_noise_stddev = *options.noise;
    }
}

/* Applies the deadline-policy override selected by name. */
void apply_deadline_override(HilConfig& config, std::string_view deadline_name)
{
    if (deadline_name == "Fail") {
        config.deadline_policy = DeadlinePolicy::Fail;
    }
    else if (deadline_name == "Abort") {
        config.deadline_policy = DeadlinePolicy::Abort;
    }
    else if (deadline_name == "Warn") {
        config.deadline_policy = DeadlinePolicy::Warn;
    }
}

HilConfig hil_config_from_options(const HilCliOptions& options)
{
    const HilScenarioRecord* record = HilScenarioCatalog::find(options.scenario_id);
    HilConfig                config = record ? record->config : hil_base_config();

    config.scenario_id = options.scenario_id;
    const Stm32DiscoveryResult discovered = discover_stm32_port(config.fc1_stlink_serial);
    resolve_hil_interface(config, options.interface_name, discovered);
    apply_numeric_overrides(config, options);
    apply_deadline_override(config, options.deadline_name);

    return config;
}

sim::sil::FaultScenario hil_fault_from_options(const HilCliOptions& options)
{
    const HilScenarioRecord* record = HilScenarioCatalog::find(options.scenario_id);

    return record ? record->fault : sim::sil::FaultScenario{};
}

}
