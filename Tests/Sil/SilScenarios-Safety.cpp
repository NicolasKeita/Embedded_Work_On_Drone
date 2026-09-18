/*
Filename: Tests/Sil/SilScenarios-Safety.cpp
Description: SIL scenario for the shared altitude-sensor fault and its degraded handling.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import Aircraft;
import FlightController;
import FunctionalScenarios;
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
using sim::sil::SensorCorruptionMode;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios);
sim::sil::FaultScenario make_sil_fault(std::string_view id,
                                       std::float64_t   start_time,
                                       std::float64_t   duration);

/* SIL fault activation timing: property of the accelerated SIL environment, not the scenario identity. */
void sensor_fault_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    const sim::test::FunctionalScenario& shared = *sim::test::find_functional_scenario("FAULT_INJECTOR-003");

    runner.begin_scenario(shared.id, shared.description);
    const FaultScenario scenario = make_sil_fault(shared.id, 20.0, 10.0);
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = shared.id, .scenario = scenario, .result = SimulationResult{}};
        return;
    }
    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_detection_event == DetectionEvent::SENSOR_VALIDATION_FAILED,
                 "altitude sensor invalidated by validation");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.50, "immediate invalidation (<= 500 ms)");
    runner.check(r.degraded_reached, "HealthMonitor in DEGRADED state");
    runner.check(r.compensated_reached, "COMPENSATED mode engaged");
    runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
    record = {.name = shared.id, .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

}
