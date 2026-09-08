/*
Filename: Tests/Sil/SilScenarios-Core.cpp
Description: SIL scenarios: nominal station-keeping, FC1 failure and communication loss.

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
void nominal_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("NOMINAL-001", "Nominal station-keeping mission (no fault)");
    const FaultScenario scenario{};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "NOMINAL-001", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(false);
    runner.check(r.mission_success, "mission completed COMPLETE with no fault");
    runner.check(r.final_state == sim::control::MissionState::COMPLETE, "terminal state COMPLETE");
    runner.check(r.final_safety_mode == SafetyMode::NORMAL, "safety mode NORMAL");
    runner.check(!r.fault_detected, "no fault detected");
    runner.check(r.max_altitude_error_m <= 10.5, "altitude error bounded (<= 10.5 m)");
    record = {.name = "NOMINAL-001", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

void fc1_failure_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FAULT_INJECTOR-001", "FC1 failure injected at t = 20.0 s");
    const FaultScenario scenario{.start_time = 20.0, .duration = 0.0, .failure_mode = FailureMode::FC1_UNAVAILABLE};
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

void communication_loss_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FAULT_INJECTOR-002", "Communication loss injected at t = 20.0 s");
    const FaultScenario scenario{.start_time = 20.0, .duration = 0.0, .failure_mode = FailureMode::FC_COMMUNICATION_LOSS};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FAULT_INJECTOR-002", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_detection_event == DetectionEvent::COMMUNICATION_TIMEOUT,
                 "COMMUNICATION_TIMEOUT detection raised");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "alert raised within 300 ms");
    runner.check(r.safe_mode_reached, "safety reaction engaged");
    runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED (step 10 rule)");
    record = {.name = "FAULT_INJECTOR-002", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}
}
