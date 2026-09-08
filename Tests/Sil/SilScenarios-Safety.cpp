/*
Filename: Tests/Sil/SilScenarios-Safety.cpp
Description: SIL scenarios for the sensor and actuator faults (FAULT_INJECTOR-003, FAULT_INJECTOR-004):
degraded handling with HealthMonitor DEGRADED and COMPENSATED thrust margin.

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
using sim::sil::SensorCorruptionMode;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios);

void sensor_fault_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FAULT_INJECTOR-003", "Altitude sensor corruption injected at t = 20.0 s");
    const FaultScenario scenario{ .start_time = 20.0, .duration = 10.0, .failure_mode = FailureMode::INVALID_SENSOR_DATA,
        .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange, .corrupted_altitude_m = 99999.0}};
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FAULT_INJECTOR-003", .scenario = scenario, .result = SimulationResult{}};
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
    record = {.name = "FAULT_INJECTOR-003", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

void actuator_degradation_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FAULT_INJECTOR-004", "Actuator efficiency 0.6 injected at t = 15.0 s");
    const FaultScenario scenario{.start_time = 15.0, .duration = 0.0, .failure_mode = FailureMode::ACTUATOR_DEGRADED,
                                 .parameters = {.efficiency = 0.6}};
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FAULT_INJECTOR-004", .scenario = scenario, .result = SimulationResult{}};
        return;
    }
    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_detection_event == DetectionEvent::ACTUATOR_MISMATCH,
                 "command/response mismatch detected");
    runner.check(r.degraded_reached, "HealthMonitor in DEGRADED state");
    runner.check(r.compensated_reached, "compensation command engaged");
    runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
    record = {.name = "FAULT_INJECTOR-004", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

}
