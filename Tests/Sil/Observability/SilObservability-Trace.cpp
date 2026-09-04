/*
Filename: Tests/Sil/Observability/SilObservability-Trace.cpp
Description: End-to-end scenario reconstruction from the event trace.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservability;

import std;

import FlightController;
import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

namespace sim::test::sil {

using sim::control::MissionState;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

namespace {

struct HeartbeatScan {
    bool sent_before = false;
    bool sent_after = false;
    const SilEvent* injected = nullptr;
};

/*
Inspects the trace around the FAULT_INJECTED marker: heartbeats must be present
before the injection and completely absent afterwards (FC1 gone silent).
*/
HeartbeatScan scan_heartbeat_around_fault(std::span<const SilEvent> events)
{
    HeartbeatScan scan{.injected = find_first(events, SilEventType::FaultInjected)};
    const std::size_t fault_index =
        scan.injected == nullptr ? 0u : static_cast<std::size_t>(scan.injected - events.data());

    for (std::size_t index = 0; index < events.size(); ++index) {
        if (events[index].type != SilEventType::HeartbeatSent) {
            continue;
        }
        if (index < fault_index) {
            scan.sent_before = true;
        }
        else {
            scan.sent_after = true;
        }
    }
    return scan;
}

}

/*
OBS-010: the FC1 failure scenario is fully reconstructible from the trace alone.
*/
void trace_reconstruction_test(TestHarness& runner)
{
    const SilConfig config{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-010 : moteur SIL en echec");
        return;
    }

    const std::vector<SilEvent>& events = outcome.value().events;
    const HeartbeatScan scan = scan_heartbeat_around_fault(events);
    const std::size_t fault_index =
        scan.injected == nullptr ? 0u : static_cast<std::size_t>(scan.injected - events.data());

    runner.check(scan.injected != nullptr, "OBS-010 : FAULT_INJECTED present");
    runner.check(scan.sent_before, "OBS-010 : heartbeats avant l'injection");
    runner.check(!scan.sent_after, "OBS-010 : heartbeats s'arrettent apres injection");
    runner.check(find_first_after(events, SilEventType::FaultDetected, fault_index) != nullptr,
                 "OBS-010 : FAULT_DETECTED apres injection");
    runner.check(find_first_after(events, SilEventType::WatchdogTimeout, fault_index) != nullptr,
                 "OBS-010 : WATCHDOG_TIMEOUT apres injection");
    runner.check(find_first_after(events, SilEventType::SafetyResponse, fault_index) != nullptr,
                 "OBS-010 : SAFETY_RESPONSE apres injection");
    runner.check(outcome.value().result.final_state == MissionState::ABORTED, "OBS-010 : mission ABORTED reconstruite");
}

}
