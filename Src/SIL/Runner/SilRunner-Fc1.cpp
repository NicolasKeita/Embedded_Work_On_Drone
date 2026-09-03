/*
Filename: Src/SIL/Runner/SilRunner-Fc1.cpp
Description: FC1 pipeline step consuming the simulated sensor chain.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import Aircraft;
import CommsBus;
import FlightController;
import SafetyManager;
import SilEvents;
import SilRunnerEvents;
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::SafetyMode;

/*
FC1 step: sensor chain acquisition with corruption applied by the environment,
FC1-side validation (hold of the last valid measurement on invalid data),
flight command computation and sequenced heartbeat emission on the bus while
the flight computer is alive. The controller only ever consumes the sensor
path, never the physics ground truth. The physics state is captured at the
exact sensor read instant so the ground-truth stream stays time-aligned with
the sensor stream.
*/
void SILRunner::update_fc1(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;

    ctx.sampled_truth = ctx.aircraft.state();
    ctx.telemetry = make_telemetry(ctx.aircraft.state());
    if (ctx.env.sensor_corruption != SensorCorruptionMode::None) {
        ctx.telemetry = apply_corruption(ctx.telemetry, ctx.env.sensor_corruption, ctx.env.corrupted_altitude_m);
    }
    if (validate(ctx.telemetry, cfg.sensor_limits).all_valid()) {
        ctx.fc1_view = to_aircraft_state(ctx.telemetry);
    }
    if (ctx.env.fc1_alive && ctx.safety.mode() != SafetyMode::SAFE_MODE) {
        ctx.command = ctx.fc1.update(cfg.target, ctx.fc1_view, cfg.dt);
    }
    if (ctx.env.fc1_alive) {
        const CommsDelivery delivery = ctx.comms.publish(ctx.time);
        ctx.comms_stats.record(delivery);
        record_heartbeat(ctx, delivery);
    }
}

}
