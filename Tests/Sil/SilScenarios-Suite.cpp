/*
Filename: Tests/Sil/SilScenarios-Suite.cpp
Description: SIL mode-change FC1 failure scenario (MC-FINJ-001) and the suite
orchestration: the six scenarios, the observability suites and the report generation.

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
import SilObservability;
import SilObservabilityTelemetry;
import SilReporting;
import SilRunner;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::safety::FaultDomain;
using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::ScenarioRecord;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios);
void nominal_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record);
void fc1_failure_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record);
void communication_loss_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record);
void sensor_fault_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record);
void actuator_degradation_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record);

void fc1_failure_during_climb_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("MC-FINJ-001_Fc1FailureDuringClimb",
                          "FC1 failure injected during the climb mode-change transition");
    const FaultScenario scenario{.start_time = 2.0, .duration = 0.0, .fault_type = FaultType::FC1Failure};
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "MC-FINJ-001_Fc1FailureDuringClimb", .scenario = scenario, .result = SimulationResult{}};
        return;
    }
    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected, "FC1 failure detected during the climb transition");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "heartbeat timeout within 300 ms");
    runner.check(r.final_safety_mode == SafetyMode::SAFE_MODE, "SAFE_MODE engaged");
    runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED by the SafetyManager");
    runner.check(r.fault_injected_time <= 5.0, "fault injected during the climb phase");
    runner.check(!r.mission_success && r.test_verdict, "verdict PASS with mission not successful");
    record = {.name = "MC-FINJ-001_Fc1FailureDuringClimb", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

/* Runs the six scenarios, the observability suite and the report artifacts. */
void run_all_sil_scenarios(TestHarness& runner)
{
    std::array<SilRunOutput, 6>   outputs{};
    std::array<ScenarioRecord, 6> records{};

    nominal_scenario(runner, outputs[0], records[0]);
    fc1_failure_scenario(runner, outputs[1], records[1]);
    communication_loss_scenario(runner, outputs[2], records[2]);
    sensor_fault_scenario(runner, outputs[3], records[3]);
    actuator_degradation_scenario(runner, outputs[4], records[4]);
    fc1_failure_during_climb_scenario(runner, outputs[5], records[5]);

    run_observability_scenarios(runner);
    run_telemetry_scenarios(runner);

    std::cout << "\n=== SIL report generation (docs/validation/data) ===" << std::endl;
    const std::expected<void, sim::sil::ReportError> outcome = sim::sil::write_sil_report(records);
    if (!outcome.has_value()) {
        runner.check(false, "SIL report generation failed");
        return;
    }
    std::cout << "Artifacts generated: sil.md, json, csv, trace jsonl, telemetry csv, truth csv" << std::endl;
    sim::sil::write_markdown_report(std::cout, records);
}

}
