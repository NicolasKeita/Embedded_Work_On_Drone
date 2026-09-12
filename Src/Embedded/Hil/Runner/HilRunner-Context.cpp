/*
Filename: Src/Embedded/Hil/Runner/HilRunner-Context.cpp
Description: HilRunner constructor and the scenario-to-context factory of the HIL runner
(building fault injectors and the host FC target / wall clock).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilRunner;

import std;

import FaultInjectors;
import FlightController;
import HilClock;
import HilConfig;
import HilFcTarget;
import HilRunnerContext;
import LoopbackTransport;
import SafetyManager;
import SerialTransport;
import SilFaultScenario;
import SilTypes;
import Transport;

namespace sim::hil {

HilRunner::HilRunner(HilConfig config) : config_{std::move(config)} {}

/* Reports whether any requested scenario carries an injected fault. */
bool any_fault_expected(std::span<const sim::sil::FaultScenario> scenarios)
{
    for (const auto& scenario : scenarios) {
        if (scenario.failure_mode != sim::sil::FailureMode::NONE) {
            return true;
        }
    }
    return false;
}

/* Selects physical FC2 supervision whenever the HIL target is a serial STM32. */
bool uses_embedded_fc2_supervision(const HilRunContext& ctx) noexcept
{
    return ctx.config.interface_name != "loopback";
}

/* Records the failure mode of one scenario when no mode was recorded yet. */
void track_failure_mode(HilRunContext& ctx, sim::sil::FailureMode mode)
{
    if (ctx.result.failure_mode == sim::sil::FailureMode::NONE) {
        ctx.result.failure_mode = mode;
    }
}

/* Installs one fault injector, rejecting the run when the scenario is invalid. */
std::expected<void, HilError> install_injector(HilRunContext& ctx, const sim::sil::FaultScenario& scenario)
{
    const std::expected<sim::sil::FaultInjector, sim::sil::InjectorError> made =
        sim::sil::make_fault_injector(scenario);

    if (!made.has_value()) {
        return std::unexpected(HilError::FaultScenarioRejected);
    }
    ctx.injectors[ctx.injector_count] = std::move(made).value();
    ctx.injector_count += 1;
    track_failure_mode(ctx, scenario.failure_mode);
    return {};
}

/* Installs the fault injectors of a run into a fresh context. */
std::expected<void, HilError> install_injectors(HilRunContext& ctx,
                                                std::span<const sim::sil::FaultScenario> scenarios)
{
    for (const sim::sil::FaultScenario& scenario : scenarios) {
        if (scenario.failure_mode == sim::sil::FailureMode::NONE) {
            continue;
        }
        if (std::expected<void, HilError> installed = install_injector(ctx, scenario); !installed) {
            return std::unexpected(installed.error());
        }
    }
    return {};
}

std::expected<std::unique_ptr<HilRunContext>, HilError>
HilRunner::makeContext(const HilConfig& config, std::span<const sim::sil::FaultScenario> scenarios)
{
    if (scenarios.size() > kHilMaxFaultScenarios) {
        return std::unexpected(HilError::TooManyScenarios);
    }
    std::expected<std::unique_ptr<FlightCore::Transport::ITransport>, HilError> channel = open_hil_channel(config);
    if (!channel.has_value()) {
        return std::unexpected(channel.error());
    }
    auto ctx = std::make_unique<HilRunContext>(config, std::move(channel).value());
    if (std::expected<void, HilError> installed = install_injectors(*ctx, scenarios); !installed) {
        return std::unexpected(installed.error());
    }
    attach_fc_target(*ctx, config);
    ctx->fault_expected = any_fault_expected(scenarios);
    return ctx;
}

}
