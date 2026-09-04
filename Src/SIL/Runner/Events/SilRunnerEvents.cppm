/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents.cppm
Description: Event recording free functions used by the SIL runner to build the structured trace.
Exports:
    record_run_start(),
    record_run_end(),
    record_fc1_failure(),
    record_heartbeat(),
    record_heartbeat_delivered(),
    record_heartbeat_dropped(),
    record_watchdog_and_detection(),
    record_watchdog_events(),
    record_recovery_start(),
    record_detection(),
    record_health_transition(),
    record_recovery_end(),
    record_safety_transitions(),
    record_mission_transition(),
    record_fault_activation(),
    record_fault_cleared()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunnerEvents;

import std;

import CommsBus;
import FlightController;
import HealthMonitor;
import SilRunnerContext;
import SilTypes;

export namespace sim::sil {

void record_run_start(RunContext& ctx);
void record_run_end(RunContext& ctx);
void record_fc1_failure(RunContext& ctx);
void record_heartbeat(RunContext& ctx, const CommsDelivery& delivery);
void record_heartbeat_delivered(RunContext& ctx, const CommsDelivery& delivery);
void record_heartbeat_dropped(RunContext& ctx, const CommsDelivery& delivery);
void record_watchdog_and_detection(RunContext& ctx, const sim::safety::HealthReport& report);
void record_watchdog_events(RunContext& ctx, const sim::safety::HealthReport& report);
void record_recovery_start(RunContext& ctx, const sim::safety::HealthReport& report);
void record_detection(RunContext& ctx, const sim::safety::HealthReport& report);
void record_health_transition(RunContext& ctx, sim::safety::HealthState current);
void record_recovery_end(RunContext& ctx, sim::safety::HealthState current);
void record_safety_transitions(RunContext& ctx);
void record_mission_transition(RunContext& ctx, sim::control::MissionState current);
void record_fault_activation(RunContext& ctx, const FaultScenario& scenario);
void record_fault_cleared(RunContext& ctx);

}

namespace sim::sil {

std::string_view fault_effect_reason(const FaultScenario& scenario);

}
