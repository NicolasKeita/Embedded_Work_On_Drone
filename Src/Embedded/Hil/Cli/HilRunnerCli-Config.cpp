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

namespace {
    /* Selects the safe loopback fallback associated with a failed hardware discovery. */
    void apply_discovery_fallback(HilConfig& config, Stm32DiscoveryStatus status)
    {
        config.interface_name = "loopback";
        if (status == Stm32DiscoveryStatus::Ambiguous) {
            config.interface_selection = InterfaceSelection::AutoFallbackAmbiguous;
        }
        else if (status == Stm32DiscoveryStatus::Unavailable) {
            config.interface_selection = InterfaceSelection::AutoFallbackUnavailable;
        }
        else {
            config.interface_selection = InterfaceSelection::AutoFallbackNotFound;
        }
    }

    /* Resolves a user path and a stable by-id path to the same physical TTY. */
    [[nodiscard]] bool same_device(std::string_view left, std::string_view right)
    {
        std::error_code error{};
        const std::filesystem::path left_path = std::filesystem::weakly_canonical(left, error);
        if (error) {
            return false;
        }
        const std::filesystem::path right_path = std::filesystem::weakly_canonical(right, error);
        return !error && left_path == right_path;
    }
}

HilConfig hil_config_from_options(const HilCliOptions& options)
{
    const HilScenarioRecord* record = HilScenarioCatalog::find(options.scenario_id);
    HilConfig                config = record ? record->config : hil_base_config();

    config.scenario_id = options.scenario_id;
    const Stm32DiscoveryResult discovered = discover_stm32_port(config.fc1_stlink_serial);
    if (options.interface_name == "auto") {
        if (discovered.status == Stm32DiscoveryStatus::Matched) {
            config.interface_name = discovered.device_path;
            config.interface_selection = InterfaceSelection::AutoTrustedStm32;
        }
        else {
            apply_discovery_fallback(config, discovered.status);
        }
    }
    else if (options.interface_name == "loopback") {
        config.interface_name = "loopback";
        config.interface_selection = InterfaceSelection::ExplicitLoopback;
    }
    else if (discovered.status == Stm32DiscoveryStatus::Matched
             && same_device(options.interface_name, discovered.device_path)) {
        config.interface_name = discovered.device_path;
        config.interface_selection = InterfaceSelection::ExplicitSerial;
    }
    else {
        config.interface_name = "loopback";
        config.interface_selection = InterfaceSelection::RejectedUntrustedDevice;
    }

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
    if (options.deadline_name == "Fail") {
        config.deadline_policy = DeadlinePolicy::Fail;
    }
    else if (options.deadline_name == "Abort") { config.deadline_policy = DeadlinePolicy::Abort; }
    else if (options.deadline_name == "Warn") { config.deadline_policy = DeadlinePolicy::Warn; }

    return config;
}

sim::sil::FaultScenario hil_fault_from_options(const HilCliOptions& options)
{
    const HilScenarioRecord* record = HilScenarioCatalog::find(options.scenario_id);

    return record ? record->fault : sim::sil::FaultScenario{};
}

}
