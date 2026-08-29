/*
Filename: Src/SIL/SilRunner-Context.cpp
Description: SIL run context assembly, environment injection and FC1 pipeline step.

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
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::SafetyMode;
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
      safety{SafetyManagerConfig{.degraded_thrust_margin = cfg.thrust_compensation_margin}} {}

SILRunner::RunContext SILRunner::make_context(const SilConfig&                  config,
                                              const std::vector<FaultScenario>& scenarios)
{
    RunContext ctx{config};
    ctx.injectors.reserve(scenarios.size());
    for (const FaultScenario& scenario : scenarios) {
        std::unique_ptr<IFaultInjector> injector = make_fault_injector(scenario);
        if (injector) {
            ctx.injectors.push_back(std::move(injector));
        }
    }
    return ctx;
}

/*
Injection step: the environment restarts from a nominal state at each step, then
each polymorphic injector alters the domain it owns. The time of the first
active fault is recorded in the result.
*/
void SILRunner::apply_injectors(RunContext& ctx)
{
    ctx.env = SimulationState{};
    bool injected = false;
    for (const std::unique_ptr<IFaultInjector>& injector : ctx.injectors) {
        injector->inject(ctx.env, ctx.time);
        injected = injected || injector->is_active(ctx.time);
    }
    if (injected && ctx.result.fault_injected_time < 0.0) {
        ctx.result.fault_injected_time = ctx.time;
    }
    ctx.comms.set_link(ctx.env.comms_link_up, ctx.env.comms_loss_probability);
}

/*
FC1 step: sensor acquisition with validation (the FC1 secondary estimator keeps
the last valid value), flight command computation, then heartbeat and status
message emission on the bus, only while FC1 is alive.
*/
void SILRunner::update_fc1(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;
    ctx.telemetry = make_telemetry(ctx.aircraft.state());
    if (ctx.env.sensor_corruption != SensorCorruptionMode::None) {
        ctx.telemetry =
            apply_corruption(ctx.telemetry, ctx.env.sensor_corruption, ctx.env.corrupted_altitude_m);
    }
    if (validate(ctx.telemetry, cfg.sensor_limits).all_valid()) {
        ctx.fc1_view = ctx.aircraft.state();
    }
    if (ctx.env.fc1_alive && ctx.safety.mode() != SafetyMode::SAFE_MODE) {
        ctx.command = ctx.fc1.update(cfg.target, ctx.fc1_view, cfg.dt);
    }
    if (ctx.env.fc1_alive) {
        static_cast<void>(ctx.comms.publish(ctx.time));
    }
}

}
