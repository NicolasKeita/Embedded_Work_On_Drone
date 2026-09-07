/*
Filename: Tests/Sil/SilScenarios-Suite.cpp
Description: SIL FC1-failure-during-climb scenario (FAULT_INJECTOR-001) and the suite
orchestration: the scenarios, the observability suites and the report generation.

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
    runner.begin_scenario("FAULT_INJECTOR-001",
                          "FC1 failure injected during the climb mode-change transition");
    const FaultScenario scenario{.start_time = 2.0, .duration = 0.0, .fault_type = FaultType::FC1Failure};
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
    runner.check(r.fault_detected, "FC1 failure detected during the climb transition");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "heartbeat timeout within 300 ms");
    runner.check(r.final_safety_mode == SafetyMode::SAFE_MODE, "SAFE_MODE engaged");
    runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED by the SafetyManager");
    runner.check(r.fault_injected_time <= 5.0, "fault injected during the climb phase");
    runner.check(!r.mission_success && r.test_verdict, "verdict PASS with mission not successful");
    record = {.name = "FAULT_INJECTOR-001", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

const std::array<SilScenarioEntry, 6> kSilScenarios{{
    {"NOMINAL-001",                  "Nominal station-keeping mission (no fault)", nominal_scenario},
    {"FAULT_INJECTOR-001",           "FC1 failure injected at t = 20.0 s",        fc1_failure_scenario},
    {"FAULT_INJECTOR-002",           "Communication loss injected at t = 20.0 s", communication_loss_scenario},
    {"FAULT_INJECTOR-003",           "Altitude sensor corruption at t = 20.0 s",  sensor_fault_scenario},
    {"FAULT_INJECTOR-004",           "Actuator efficiency 0.6 at t = 15.0 s",     actuator_degradation_scenario},
    {"FAULT_INJECTOR-001", "FC1 failure during the climb transition", fc1_failure_during_climb_scenario},
}};

std::span<const SilScenarioEntry> sil_scenarios() noexcept
{
    return kSilScenarios;
}

const SilScenarioEntry* find_sil_scenario(std::string_view id) noexcept
{
    for (const SilScenarioEntry& entry : kSilScenarios) {
        if (entry.id == id) {
            return &entry;
        }
    }
    return nullptr;
}

}
