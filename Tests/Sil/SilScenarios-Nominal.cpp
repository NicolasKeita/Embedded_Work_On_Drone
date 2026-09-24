/*
Filename: Tests/Sil/SilScenarios-Nominal.cpp
Description: Nominal and wind-disturbance mission scenarios of the SIL test suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import FlightController;
import FunctionalScenarios;
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

std::expected<SilRunOutput, SilError> run_case(std::string_view id, std::span<const FaultScenario> scenarios);

/* Executes the canonical no-fault mission through the configured SIL pipeline. */
void nominal_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    const sim::test::FunctionalScenario& scenario = *sim::test::find_functional_scenario("NOMINAL-001");

    runner.begin_scenario(scenario.id, scenario.description);
    const FaultScenario fault{};
    const std::array<FaultScenario, 1> scenarios{fault};

    const std::expected<SilRunOutput, SilError> outcome = run_case(scenario.id, scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = scenario.id, .scenario = fault, .result = SimulationResult{}};
        return;
    }

    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(false);
    runner.check(r.mission_success, "mission completed COMPLETE with no fault");
    runner.check(r.final_state == sim::control::MissionState::COMPLETE, "terminal state COMPLETE");
    runner.check(r.final_safety_mode == SafetyMode::NORMAL, "safety mode NORMAL");
    runner.check(!r.fault_detected, "no fault detected");
    runner.check(r.max_altitude_error_m <= std::abs(scenario.target.z) + 0.5,
                 "altitude error bounded by target altitude plus 0.5 m");
    record = {.name = scenario.id, .scenario = fault, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

/* Executes one configured wind mission and verifies recovery without safety faults. */
void run_wind_scenario(std::string_view id, TestHarness& runner,
                       SilRunOutput& output, ScenarioRecord& record)
{
    const auto& scenario = *sim::test::find_functional_scenario(id);
    runner.begin_scenario(scenario.id, scenario.description);
    const FaultScenario fault{};
    const std::array<FaultScenario, 1> scenarios{fault};
    auto outcome = run_case(scenario.id, scenarios);
    if (!outcome) {
        runner.check(false, "SIL wind runner failed");
        output = SilRunOutput{};
        record = {.name = scenario.id, .scenario = fault, .result = SimulationResult{}};
        return;
    }
    output = std::move(*outcome);
    auto& result = output.result;
    const bool recovered = std::abs(result.final_x_m - scenario.target.x) <= scenario.tracking_tolerance
        && std::abs(result.final_y_m - scenario.target.y) <= scenario.tracking_tolerance
        && std::abs(result.final_altitude_m - scenario.target.z) <= scenario.tracking_tolerance;
    result.test_verdict = result.compute_verdict(false) && recovered;
    runner.check(result.mission_success, "wind mission completes");
    runner.check(!result.fault_detected && result.final_safety_mode == SafetyMode::NORMAL,
                 "wind disturbance does not cause a safety fault");
    runner.check(recovered, "configured target recovered within tracking tolerance");
    runner.check(std::isfinite(result.max_position_error_m) && std::isfinite(result.max_altitude_error_m),
                 "wind response remains finite");
    record = {.name = scenario.id, .scenario = fault, .result = result, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

/* Executes the canonical steady crosswind through the complete SIL control and safety pipeline. */
void steady_wind_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    run_wind_scenario("WIND-001", runner, output, record);
}

/* Executes the canonical diagonal gusts through the complete SIL control and safety pipeline. */
void gust_wind_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    run_wind_scenario("WIND-002", runner, output, record);
}

}
