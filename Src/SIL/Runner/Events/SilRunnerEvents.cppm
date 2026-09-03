/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents.cppm
Description: Event recording free functions used by the SIL runner to build the structured trace.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunnerEvents;

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