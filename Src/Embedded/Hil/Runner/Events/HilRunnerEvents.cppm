/*
Filename: Src/Embedded/Hil/Runner/Events/HilRunnerEvents.cppm
Description: Structured event recording of the HIL runner : run lifecycle, mission and
safety transitions, fault injection/clearing, FC1 failure and heartbeat events. Mirrors
the SilRunnerEvents module of the validated SIL baseline.
Exports:
    record_run_start(),
    record_run_end(),
    record_mission_transition(),
    record_safety_transitions(),
    record_detection(),
    record_fault_events(),
    record_fc1_failure_event(),
    record_step_error()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunnerEvents;

import std;

import FlightController;
import HealthMonitor;
import HilRunnerContext;
import HilTransport;

export namespace sim::hil {

void record_run_start(HilRunContext& ctx);
void record_run_end(HilRunContext& ctx);
void record_mission_transition(HilRunContext& ctx, sim::control::MissionState current);
void record_safety_transitions(HilRunContext& ctx);
void record_detection(HilRunContext& ctx, const sim::safety::HealthReport& report);
void record_fault_events(HilRunContext& ctx);
void record_fc1_failure_event(HilRunContext& ctx);
void record_step_error(HilRunContext& ctx, ReceiveResult result);

}