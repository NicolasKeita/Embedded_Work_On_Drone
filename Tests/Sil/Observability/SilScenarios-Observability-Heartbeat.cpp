/*
Filename: Tests/Sil/Observability/SilScenarios-Observability-Heartbeat.cpp
Description: Heartbeat sequence and latency observability test.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

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
    bool sequences_ok = true;
    bool latency_ok = true;
    std::uint64_t last_sequence = 0;
    bool first = true;

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
    const SilConfig config{.duration_s = 5.0, .trace_level = SilLogLevel::Trace};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, FaultScenario{});

    if (!outcome.has_value()) {
        runner.check(false, "OBS-001 : moteur SIL en echec");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t sent = count_events(output.events, SilEventType::HeartbeatSent);
    const std::size_t delivered = count_events(output.events, SilEventType::HeartbeatDelivered);

    runner.check(sent > 0 && sent == delivered, "OBS-001 : chaque heartbeat envoye est delivre");
    runner.check(check_heartbeat_trace(output.events), "OBS-001 : sequences et latences valides");

    const SilConfig quiet_config{.duration_s = 5.0};
    const std::expected<SilRunOutput, SilError> quiet_outcome = run_traced(quiet_config, FaultScenario{});

    if (!quiet_outcome.has_value()) {
        runner.check(false, "OBS-001 : moteur SIL (niveau info) en echec");
        return;
    }
    const SilRunOutput& quiet = quiet_outcome.value();
    runner.check(count_events(quiet.events, SilEventType::HeartbeatSent) == 0
                     && count_events(quiet.events, SilEventType::SimulationStart) == 1,
                 "OBS-001 : le niveau info filtre le trafic heartbeat");
}

}