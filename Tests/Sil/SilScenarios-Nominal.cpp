/*
Filename: Tests/Sil/SilScenarios-Nominal.cpp
Description: Nominal station-keeping mission scenario (NOMINAL-001) of the SIL test suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import FlightController;
import SafetyManager;
import SilTypes;
import TestHarness;

namespace sim::test::sil {

using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::ScenarioRecord;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios);

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

}
