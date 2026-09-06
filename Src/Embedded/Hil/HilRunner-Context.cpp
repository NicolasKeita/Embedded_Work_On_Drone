/*
Filename: Src/Embedded/Hil/HilRunner-Context.cpp
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
import SafetyManager;
import SilFaultScenario;
import SilTypes;

namespace sim::hil {

HilRunner::HilRunner(HilConfig config) : config_{std::move(config)} {}

bool any_fault_expected(std::span<const sim::sil::FaultScenario> scenarios)
{
    for (const auto& scenario : scenarios) {
        if (scenario.fault_type != sim::sil::FaultType::None) {
            return true;
        }
    }
    return false;
}

std::expected<std::unique_ptr<HilRunContext>, HilError> HilRunner::makeContext(const HilConfig& config,
                                                                                 std::span<const sim::sil::FaultScenario> scenarios)
{
    if (scenarios.size() > kHilMaxFaultScenarios) {
        return std::unexpected(HilError::TooManyScenarios);
    }

    auto ctx = std::make_unique<HilRunContext>(config);

    for (const sim::sil::FaultScenario& scenario : scenarios) {
        if (scenario.fault_type == sim::sil::FaultType::None) {
            continue;
        }
        const std::expected<sim::sil::FaultInjector, sim::sil::InjectorError> made =
            sim::sil::make_fault_injector(scenario);
        if (!made.has_value()) {
            return std::unexpected(HilError::FaultScenarioRejected);
        }
        ctx->injectors[ctx->injector_count] = std::move(made).value();
        ctx->injector_count += 1;
        if (ctx->result.fault_type == sim::sil::FaultType::None) {
            ctx->result.fault_type = scenario.fault_type;
        }
    }

    if (config.clock_kind == ClockKind::Monotonic) {
        ctx->clock = std::make_unique<MonotonicClock>();
    }
    else {
        ctx->clock = std::make_unique<FastClock>();
    }
    ctx->fc_target = std::make_unique<HostFcTarget>(ctx->channel, *ctx->clock, config.target,
                                                     config.controller, config.dt_s, config.sensor_limits);
    ctx->fault_expected = any_fault_expected(scenarios);
    return ctx;
}

}

