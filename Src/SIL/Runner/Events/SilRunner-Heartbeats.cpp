/*
Filename: Src/SIL/Runner/Events/SilRunner-Heartbeats.cpp
Description: Heartbeat send/deliver/drop event recording for the detailed trace.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunner;

import std;

import CommsBus;
import SilEvents;

namespace sim::sil {

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
