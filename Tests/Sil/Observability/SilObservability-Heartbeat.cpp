/*
Filename: Tests/Sil/Observability/SilObservability-Heartbeat.cpp
Description: Heartbeat sequence and latency observability test.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservability;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultScenario;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

namespace {

/*
Validates that every sent heartbeat carries a strictly increasing sequence and
that every delivery records its latency.
*/
bool check_heartbeat_trace(std::span<const SilEvent> events)
{
    bool          sequences_ok = true;
    bool          latency_ok = true;
    std::uint64_t last_sequence = 0;
    bool          first = true;

    for (const SilEvent& event : events) {
        if (event.type == SilEventType::HeartbeatSent) {
            if (!event.has_sequence || (!first && event.sequence <= last_sequence)) {
                sequences_ok = false;
            }
            last_sequence = event.sequence;
            first = false;
        }
        if (event.type == SilEventType::HeartbeatDelivered
            && (!event.has_latency || event.latency_s < 0.0 || !event.has_sequence)) {
            latency_ok = false;
        }
    }
    return sequences_ok && latency_ok;
}

}

/*
OBS-001: heartbeats are logged with sequence numbers and delivery latency at
trace level, and filtered out at the default info level.
*/
void heartbeat_trace_test(TestHarness& runner)
{
    runner.set_context("OBS-001");
    const SilConfig                             config{.duration_s = 5.0, .trace_level = SilLogLevel::Trace};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, FaultScenario{});

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t sent = count_events(output.events, SilEventType::HeartbeatSent);
    const std::size_t delivered = count_events(output.events, SilEventType::HeartbeatDelivered);

    runner.check(sent > 0 && sent == delivered, "every sent heartbeat is delivered");
    runner.check(check_heartbeat_trace(output.events), "valid sequences and latencies");

    const SilConfig quiet_config{.duration_s = 5.0};
    const std::expected<SilRunOutput, SilError> quiet_outcome = run_traced(quiet_config, FaultScenario{});

    if (!quiet_outcome.has_value()) {
        runner.check(false, "SIL runner failed (info level)");
        return;
    }
    const SilRunOutput& quiet = quiet_outcome.value();
    runner.check(count_events(quiet.events, SilEventType::HeartbeatSent) == 0
                     && count_events(quiet.events, SilEventType::SimulationStart) == 1,
                 "info level filters heartbeat traffic");
}

}
