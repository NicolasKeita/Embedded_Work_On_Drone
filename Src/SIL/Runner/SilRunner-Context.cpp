/*
Filename: Src/SIL/Runner/SilRunner-Context.cpp
Description: Run context assembly and environment fault injection step.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import Aircraft;
import CommsBus;
import FaultInjectors;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::HealthMonitorConfig;
using sim::safety::SafetyManagerConfig;

SILRunner::SILRunner(SilConfig config) : config_{config} {}

SILRunner::RunContext::RunContext(const SilConfig& cfg)
    : config{cfg},
      fc1{cfg.controller},
      comms{cfg.seed},
      health{HealthMonitorConfig{.heartbeat_timeout_s = cfg.heartbeat_timeout_s,
                                 .actuator_mismatch_rpm = cfg.actuator_mismatch_rpm,
                                 .sensor_limits = cfg.sensor_limits}},
      safety{SafetyManagerConfig{.degraded_thrust_margin = cfg.thrust_compensation_margin}},
      trace{SilTraceConfig{.level = cfg.trace_level}}
{
    telemetry_recorder.interval_s = cfg.telemetry_rate_hz > 0.0 ? 1.0 / cfg.telemetry_rate_hz : 0.0;
    comms.set_transport_latency(cfg.transport_latency_s);
}

/*
Builds the run context and converts each declarative scenario into a value
injector inside the fixed-capacity array. Nominal scenarios are skipped and
invalid ones are rejected with a typed error.
*/
std::expected<SILRunner::RunContext, SilError> SILRunner::make_context(const SilConfig&               config,
                                                            std::span<const FaultScenario> scenarios)
{
    if (scenarios.size() > kMaxFaultInjectors) {
        return std::unexpected(SilError::TooManyScenarios);
    }

    RunContext ctx{config};

    for (const FaultScenario& scenario : scenarios) {
        const std::expected<FaultInjector, InjectorError> injector = make_fault_injector(scenario);
        if (!injector.has_value()) {
            if (injector.error() == InjectorError::NoFault) {
                continue;
            }
            return std::unexpected(SilError::FaultScenarioRejected);
        }
        ctx.injectors[ctx.injector_count] = injector.value();
        ++ctx.injector_count;
    }
    return ctx;
}

/*
Injection step: the environment restarts from a nominal state then each value
injector alters the domain it owns. Rising edges become structured injection
events, falling edges become fault-cleared events (temporary faults); the first
activation time and type are stored in the result.
*/
void SILRunner::apply_injectors(RunContext& ctx)
{
    ctx.env = SimulationState{};
    bool injected = false;
    const FaultScenario* active_scenario = nullptr;

    for (std::size_t index = 0; index < ctx.injector_count; ++index) {
        ctx.injectors[index].inject(ctx.env, ctx.time);
        if (ctx.injectors[index].is_active(ctx.time)) {
            injected = true;
            active_scenario = &ctx.injectors[index].scenario();
        }
    }
    if (injected && !ctx.fault_active && active_scenario != nullptr) {
        ctx.fault_active = true;
        ctx.last_fault_type = active_scenario->fault_type;
        ctx.last_fault_start = ctx.time;
        record_fault_activation(ctx, *active_scenario);
    }
    else if (!injected && ctx.fault_active) {
        ctx.fault_active = false;
        record_fault_cleared(ctx);
    }
    if (injected && !ctx.fault_recorded && active_scenario != nullptr) {
        ctx.fault_recorded = true;
        ctx.result.fault_injected_time = ctx.time;
        ctx.result.fault_type = active_scenario->fault_type;
    }
    if (ctx.fc1_was_alive && !ctx.env.fc1_alive) {
        record_fc1_failure(ctx);
    }
    ctx.fc1_was_alive = ctx.env.fc1_alive;
    ctx.comms.set_link(ctx.env.comms_link_up, ctx.env.comms_loss_probability);
}

}
