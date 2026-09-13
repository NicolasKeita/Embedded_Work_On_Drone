/*
Filename: Tests/Sil/SilScenarios-Core.cpp
Description: SIL scenarios for nominal station keeping and the shared FC1 fault.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import Aircraft;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilReporting;
import SilRunner;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::safety::DetectionEvent;
using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::FailureMode;
using sim::sil::ScenarioRecord;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;
using sim::sil::SILRunner;

/* Test case: climb mission towards 10 m, optional faults applied by the engine. */
std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios)
{
    return SILRunner{}.run(scenarios);
}

sim::sil::FaultScenario make_sil_fault(std::string_view id,
                                       std::float64_t   start_time,
                                       std::float64_t   duration);

void fc1_failure_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FAULT_INJECTOR-001", "FC1 failure injected at t = 20.0 s");
    const FaultScenario scenario = make_sil_fault("FAULT_INJECTOR-001", 20.0, 0.0);
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FAULT_INJECTOR-001", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected, "FC1 failure detected");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "heartbeat timeout within 300 ms");
    runner.check(r.final_safety_mode == SafetyMode::SAFE_MODE, "SAFE_MODE engaged");
    runner.check(r.response_latency >= 0.0 && r.response_latency <= 0.20, "response latency <= 200 ms");
    runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED by the SafetyManager");
    runner.check(!r.mission_success && r.test_verdict, "verdict PASS with mission not successful");
    record = {.name = "FAULT_INJECTOR-001", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

}
