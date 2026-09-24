/*
Filename: Src/Host/Config/HilRuntimeConfig.cppm
Description: Host-only HIL runner settings, validation and effective run configuration.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRuntimeConfig;

import std;

import HilConfig;
import SilFaultScenario;

export namespace sim::host {

/* Loads the HIL host baseline before applying a scenario and command-line overrides. */
[[nodiscard]] std::expected<sim::hil::HilConfig, std::string>
load_hil_runtime_config(const std::filesystem::path& path);

/* Checks resolved values and rejects host controller overrides unsupported by physical firmware. */
[[nodiscard]] std::expected<void, std::string>
validate_hil_runtime_config(const sim::hil::HilConfig& config);

/* Records scenario and CLI values that override the resolved source documents. */
[[nodiscard]] std::string hil_configuration_text(const sim::hil::HilConfig& config,
                                                const sim::sil::FaultScenario& fault);

}
