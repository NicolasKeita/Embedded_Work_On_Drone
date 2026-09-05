/*
Filename: Tests/Sil/Observability/SilObservability-Dropped.cpp
Description: Dropped heartbeat observability test.

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
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

/*
OBS-002: dropped heartbeats are observable in the trace and in the statistics.
*/
void dropped_heartbeats_test(TestHarness& runner)
{
    const SilConfig                             config{.duration_s = 5.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario                         scenario{.start_time = 2.0, .fault_type = FaultType::CommunicationLoss};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-002 : moteur SIL en echec");
        return;
    }

    const SilRunOutput& output = outcome.value();
    const std::size_t sent = count_events(output.events, SilEventType::HeartbeatSent);
    const std::size_t delivered = count_events(output.events, SilEventType::HeartbeatDelivered);
    const std::size_t dropped = count_events(output.events, SilEventType::HeartbeatDropped);

    runner.check(dropped > 0, "OBS-002 : heartbeats perdus visibles dans la trace");
    runner.check(sent == delivered + dropped, "OBS-002 : sent = delivered + dropped");
    runner.check(output.result.comms.dropped == dropped && output.result.comms.sent == sent,
                 "OBS-002 : statistiques coherentes avec la trace");
}

}
