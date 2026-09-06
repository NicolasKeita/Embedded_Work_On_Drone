/*
Filename: Tests/Sil/SilScenarios-Safety.cpp
Description: SIL scenarios for the sensor and actuator faults (FINJ-003, FINJ-004):
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
using sim::sil::SensorCorruptionMode;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

std::expected<SilRunOutput, SilError> run_case(std::span<const FaultScenario> scenarios);

void sensor_fault_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FINJ-003_SensorFault", "Altitude sensor corruption injected at t = 20.0 s");
    const FaultScenario scenario{ .start_time = 20.0, .duration = 10.0, .fault_type = FaultType::SensorFault,
        .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange, .corrupted_altitude_m = 99999.0}};
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FINJ-003_SensorFault", .scenario = scenario, .result = SimulationResult{}};
        return;
    }
    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Sensor,
                 "altitude sensor invalidated by validation");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.50, "immediate invalidation (<= 500 ms)");
    runner.check(r.degraded_reached, "HealthMonitor in DEGRADED state");
    runner.check(r.compensated_reached, "COMPENSATED mode engaged");
    runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
    record = {.name = "FINJ-003_SensorFault", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

void actuator_degradation_scenario(TestHarness& runner, SilRunOutput& output, ScenarioRecord& record)
{
    runner.begin_scenario("FINJ-004_ActuatorDegradation", "Actuator efficiency 0.6 injected at t = 15.0 s");
    const FaultScenario scenario{.start_time = 15.0, .duration = 0.0, .fault_type = FaultType::ActuatorDegradation,
                                 .parameters = {.efficiency = 0.6}};
    const std::array<FaultScenario, 1> scenarios{scenario};
    const std::expected<SilRunOutput, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        output = SilRunOutput{};
        record = {.name = "FINJ-004_ActuatorDegradation", .scenario = scenario, .result = SimulationResult{}};
        return;
    }
    output = std::move(outcome).value();
    SimulationResult& r = output.result;
    r.test_verdict = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Actuator,
                 "command/response mismatch detected");
    runner.check(r.degraded_reached, "HealthMonitor in DEGRADED state");
    runner.check(r.compensated_reached, "compensation command engaged");
    runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
    record = {.name = "FINJ-004_ActuatorDegradation", .scenario = scenario, .result = r, .events = output.events,
              .telemetry = output.telemetry, .ground_truth = output.ground_truth};
}

}
