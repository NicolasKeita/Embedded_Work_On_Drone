/*
Filename: Src/Host/Config/SilRuntimeConfig.cppm
Description: Validated host-only SIL runtime options and shared mission defaults.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRuntimeConfig;

import std;

import FlightControllerTypes;
import SilRunnerContext;

export namespace sim::host {

struct SilRuntimeOptions {
    sim::sil::SilConfig runner{};
    std::float64_t report_period_s = 1.0;
    std::float64_t steady_window_s = 10.0;
    std::float64_t viewer_period_s = 0.05;
    std::float64_t viewer_replay_max_delay_s = 0.1;
    std::uint64_t viewer_max_samples = 600;
    bool viewer_enabled = true;
    bool report_period_cli = false;
    bool verbose = false;
};

/* Loads and validates host SIL settings without changing firmware defaults. */
[[nodiscard]] std::expected<SilRuntimeOptions, std::string> load_sil_runtime_config(std::string_view path);

/* Validates resolved mission timing, report cadence and execution limits before running. */
[[nodiscard]] std::expected<void, std::string> validate_sil_runtime_profiles(
    const SilRuntimeOptions& options, std::string_view scenario_id = {});

/* Installs validated settings before any scenario or control loop begins. */
void set_sil_runtime_options(const SilRuntimeOptions& options);

/* Returns the immutable process baseline shared by SIL host execution paths. */
[[nodiscard]] const SilRuntimeOptions& sil_runtime_options() noexcept;

/* Overlays canonical mission settings on the configured SIL runner baseline. */
[[nodiscard]] sim::sil::SilConfig sil_config_for_scenario(std::string_view id);

/* Resolves report cadence with command-line values taking precedence over profiles. */
[[nodiscard]] std::float64_t sil_report_period(std::string_view id) noexcept;

/* Produces the configured emulated controller with the model's hover reference. */
[[nodiscard]] sim::control::ControllerConfig sil_controller_config(std::float64_t hover_rpm,
                                                                  std::string_view scenario_id = {});

}
