/*
Filename: Src/SIL/Runner/SilRunner-Fc1.cpp
Description: FC1 pipeline step and heartbeat traffic event recording.

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
import SilTypes;
import Telemetry;

namespace sim::sil {

using sim::safety::SafetyMode;

/*
FC1 step: sensor acquisition with validation, flight command computation and
sequenced heartbeat emission on the bus while the flight computer is alive.
*/
void SILRunner::update_fc1(RunContext& ctx)
{
    const SilConfig& cfg = ctx.config;

    ctx.telemetry = make_telemetry(ctx.aircraft.state());
    if (ctx.env.sensor_corruption != SensorCorruptionMode::None) {
        ctx.telemetry = apply_corruption(ctx.telemetry, ctx.env.sensor_corruption, ctx.env.corrupted_altitude_m);
    }
    if (validate(ctx.telemetry, cfg.sensor_limits).all_valid()) {
        ctx.fc1_view = ctx.aircraft.state();
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

/*
Emits the heartbeat send marker then dispatches the delivery outcome event.
*/
void SILRunner::record_heartbeat(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent sent;

    sent.timestamp = delivery.send_time;
    sent.source = "FC1->FC2";
    sent.type = SilEventType::HeartbeatSent;
    sent.severity = EventSeverity::Trace;
    sent.sequence = delivery.sequence;
    sent.has_sequence = true;
    ctx.trace.record(sent);

    if (delivery.delivered) {
        record_heartbeat_delivered(ctx, delivery);
        return;
    }
    record_heartbeat_dropped(ctx, delivery);
}

/*
Emits the delivery receipt with its latency and the FC2 watchdog kick.
*/
void SILRunner::record_heartbeat_delivered(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent delivered;

    delivered.timestamp = delivery.receive_time;
    delivered.source = "FC2<-FC1";
    delivered.type = SilEventType::HeartbeatDelivered;
    delivered.severity = EventSeverity::Trace;
    delivered.sequence = delivery.sequence;
    delivered.has_sequence = true;
    delivered.latency_s = delivery.latency_s;
    delivered.has_latency = true;
    ctx.trace.record(delivered);

    SilEvent kick;

    kick.timestamp = delivery.receive_time;
    kick.source = "FC2";
    kick.type = SilEventType::WatchdogKick;
    kick.severity = EventSeverity::Debug;
    kick.sequence = delivery.sequence;
    kick.has_sequence = true;
    ctx.trace.record(kick);
}

/*
Emits the dropped heartbeat marker with its sequence number.
*/
void SILRunner::record_heartbeat_dropped(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent dropped;

    dropped.timestamp = delivery.send_time;
    dropped.source = "FC1->FC2";
    dropped.type = SilEventType::HeartbeatDropped;
    dropped.severity = EventSeverity::Trace;
    dropped.sequence = delivery.sequence;
    dropped.has_sequence = true;
    ctx.trace.record(dropped);
}

}