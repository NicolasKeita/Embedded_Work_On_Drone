/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Heartbeats.cpp
Description: Heartbeat send/deliver/drop event recording for the detailed trace.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import CommsBus;
import SilEvents;
import SilRunnerContext;

namespace sim::sil {

/*
Emits the heartbeat send marker then dispatches the delivery outcome event.
*/
void record_heartbeat(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent sent{.timestamp = delivery.send_time,
                  .source = "FC1->FC2",
                  .type = SilEventType::HeartbeatSent,
                  .severity = EventSeverity::Trace,
                  .sequence = delivery.sequence,
                  .has_sequence = true};

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
void record_heartbeat_delivered(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent delivered{.timestamp = delivery.receive_time,
                       .source = "FC2<-FC1",
                       .type = SilEventType::HeartbeatDelivered,
                       .severity = EventSeverity::Trace,
                       .sequence = delivery.sequence,
                       .has_sequence = true,
                       .latency_s = delivery.latency_s,
                       .has_latency = true};

    ctx.trace.record(delivered);

    SilEvent kick{.timestamp = delivery.receive_time,
                  .source = "FC2",
                  .type = SilEventType::WatchdogKick,
                  .severity = EventSeverity::Debug,
                  .sequence = delivery.sequence,
                  .has_sequence = true};
    ctx.trace.record(kick);
}

/*
Emits the dropped heartbeat marker with its sequence number.
*/
void record_heartbeat_dropped(RunContext& ctx, const CommsDelivery& delivery)
{
    SilEvent dropped{.timestamp = delivery.send_time,
                     .source = "FC1->FC2",
                     .type = SilEventType::HeartbeatDropped,
                     .severity = EventSeverity::Trace,
                     .sequence = delivery.sequence,
                     .has_sequence = true};

    ctx.trace.record(dropped);
}
}
