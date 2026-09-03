/*
Filename: Tests/Sil/SilScenarios-Core.cpp
Description: SIL scenarios 001 to 003 : nominal, FC1 failure and communication loss.

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

using sim::safety::FaultDomain;
using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
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
    std::cout << "\n=== SIL-001 : vol nominal sans faulte ===" << std::endl;
    const FaultScenario scenario{};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL-001 : moteur SIL en echec");
        output = SilRunOutput{};
        record = {.name = "SIL-001", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(false);
    runner.check(r.mission_success, "SIL-001 : mission COMPLETE sans faulte");
    runner.check(r.final_state == sim::control::MissionState::COMPLETE, "SIL-001 : etat terminal COMPLETE");
    runner.check(r.final_safety_mode == SafetyMode::NORMAL, "SIL-001 : mode de surete NORMAL");
    runner.check(!r.fault_detected, "SIL-001 : aucune faulte detectee");
    runner.check(r.max_altitude_error_m <= 10.5, "SIL-001 : erreur d'altitude maitrisee (<= 10.5 m)");
    record = {.name = "SIL-001", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

void fc1_failure_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    std::cout << "\n=== SIL-002 : defaillance FC1 a t = 30.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 30.0, .duration = 0.0, .fault_type = FaultType::FC1Failure};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL-002 : moteur SIL en echec");
        output = SilRunOutput{};
        record = {.name = "SIL-002", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected, "SIL-002 : defaillance FC1 detectee");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "SIL-002 : timeout heartbeat en <= 300 ms");
    runner.check(r.final_safety_mode == SafetyMode::SAFE_MODE, "SIL-002 : passage en SAFE_MODE");
    runner.check(r.response_latency >= 0.0 && r.response_latency <= 0.20, "SIL-002 : latence de reponse <= 200 ms");
    runner.check(r.final_state == sim::control::MissionState::ABORTED,
                 "SIL-002 : mission ABORTED par le SafetyManager");
    runner.check(!r.mission_success && r.test_verdict, "SIL-002 : verdict PASS avec mission non reussie");
    record = {.name = "SIL-002", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

void communication_loss_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    std::cout << "\n=== SIL-003 : perte de communication a t = 30.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 30.0, .duration = 0.0, .fault_type = FaultType::CommunicationLoss};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL-003 : moteur SIL en echec");
        output = SilRunOutput{};
        record = {.name = "SIL-003", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Communication,
                 "SIL-003 : alerte COMMUNICATION_LOST levee");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30, "SIL-003 : alerte en <= 300 ms");
    runner.check(r.safe_mode_reached, "SIL-003 : reaction de surete engagee");
    runner.check(r.final_state == sim::control::MissionState::ABORTED, "SIL-003 : mission ABORTED (regle etape 10)");
    record = {.name = "SIL-003", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}
}
