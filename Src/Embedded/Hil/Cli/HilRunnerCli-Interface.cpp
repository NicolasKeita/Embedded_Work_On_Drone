/*
Filename: Src/Embedded/Hil/Cli/HilRunnerCli-Interface.cpp
Description: Interface discovery resolution of hil_runner CLI configurations.

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

/* Resolves a user path and a stable by-id path to the same physical TTY. */
[[nodiscard]] bool same_serial_device(std::string_view left, std::string_view right)
{
    std::error_code             error{};
    const std::filesystem::path left_path = std::filesystem::weakly_canonical(left, error);

    if (error) {
        return false;
    }
    const std::filesystem::path right_path = std::filesystem::weakly_canonical(right, error);
    return !error && left_path == right_path;
}

/* Reports whether a discovery result authorizes its device path. */
[[nodiscard]] bool trusted_serial_match(const Stm32DiscoveryResult& discovered, std::string_view requested)
{
    return discovered.status == Stm32DiscoveryStatus::Matched
        && same_serial_device(requested, discovered.device_path);
}

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

/* Applies the trusted device selected by automatic discovery. */
void apply_auto_match(HilConfig& config, const Stm32DiscoveryResult& discovered)
{
    config.interface_name = discovered.device_path;
    config.interface_selection = InterfaceSelection::AutoTrustedStm32;
}

/* Resolves the trusted interface of an explicit device against the discovery. */
void resolve_explicit_interface(HilConfig&                  config,
                                std::string_view            requested,
                                const Stm32DiscoveryResult& discovered)
{
    if (trusted_serial_match(discovered, requested)) {
        config.interface_name = discovered.device_path;
        config.interface_selection = InterfaceSelection::ExplicitSerial;
        return;
    }
    config.interface_name = "loopback";
    config.interface_selection = InterfaceSelection::RejectedUntrustedDevice;
}

/* Applies the automatic interface selection to a configuration. */
void apply_auto_selection(HilConfig& config, const Stm32DiscoveryResult& discovered)
{
    if (discovered.status == Stm32DiscoveryStatus::Matched) {
        apply_auto_match(config, discovered);
        return;
    }
    apply_discovery_fallback(config, discovered.status);
}

/* Applies the explicit loopback selection to a configuration. */
void apply_loopback_selection(HilConfig& config)
{
    config.interface_name = "loopback";
    config.interface_selection = InterfaceSelection::ExplicitLoopback;
}

/* Resolves loopback/auto/explicit interface selection into the configuration. */
void resolve_hil_interface(HilConfig&                  config,
                           std::string_view            requested,
                           const Stm32DiscoveryResult& discovered)
{
    if (requested == "auto") {
        apply_auto_selection(config, discovered);
        return;
    }
    if (requested == "loopback") {
        apply_loopback_selection(config);
        return;
    }
    resolve_explicit_interface(config, requested, discovered);
}

}
