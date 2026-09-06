/*
Filename: Tests/Hil/HilTests-Faults.cpp
Description: HIL fault tests : FC1 failure, communication loss, sensor fault and actuator
degradation, asserting the fault is injected through the HIL data path, detected
naturally by the reused safety/health core and handled per the expected safety behavior.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import FlightControllerTypes;
import HealthMonitor;
import HilConfig;
import HilRunner;
import HilRunnerContext;
import HilScenarios;
import SilFaultScenario;
import TestHarness;

namespace sim::test::hil {

namespace {
    std::expected<sim::hil::HilRunOutput, sim::hil::HilError> run_scenario(std::string_view id, std::float64_t duration_s)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        sim::hil::HilConfig config = record->config;
        config.scenario_id = id;
        config.duration_s = duration_s;
        config.real_time_pacing = false;
        config.clock_kind = sim::hil::ClockKind::Fast;
        sim::hil::HilRunner runner{config};
        const std::array<sim::sil::FaultScenario, 1> scenarios{record->fault};
        return runner.run(scenarios);
    }
}

void run_fault_tests(sim::test::TestHarness& runner)
{
    {
        runner.set_context("FINJ-001_Fc1Failure");
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("FINJ-001_Fc1Failure", 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "FC1 failure detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::FC1Heartbeat,
                     "fault classified as FC1 heartbeat");
        runner.check(r.safe_mode_reached, "SAFE_MODE engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED");
        runner.check(r.test_verdict, "verdict PASS on safety behavior");
    }

    {
        runner.set_context("FINJ-002_CommLoss");
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("FINJ-002_CommLoss", 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "communication loss detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Communication,
                     "fault classified as communication");
        runner.check(r.safe_mode_reached, "safety reaction engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED");
        runner.check(r.test_verdict, "verdict PASS on safety behavior");
    }

    {
        runner.set_context("FINJ-003_SensorFault");
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("FINJ-003_SensorFault", 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "sensor fault detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Sensor,
                     "fault classified as sensor");
        runner.check(r.degraded_reached, "HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "COMPENSATED mode engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
        runner.check(r.test_verdict, "verdict PASS on degraded handling");
    }

    {
        runner.set_context("FINJ-004_ActuatorDegradation");
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("FINJ-004_ActuatorDegradation", 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "actuator degradation detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Actuator,
                     "fault classified as actuator");
        runner.check(r.degraded_reached, "HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "thrust compensation engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
        runner.check(r.test_verdict, "verdict PASS on compensation");
    }

    {
        runner.set_context("MC-FINJ-001_Fc1FailureDuringClimb");
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o =
            run_scenario("MC-FINJ-001_Fc1FailureDuringClimb", 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "FC1 failure detected during the climb transition");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::FC1Heartbeat,
                     "fault classified as FC1 heartbeat");
        runner.check(r.safe_mode_reached, "SAFE_MODE engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED");
        runner.check(r.test_verdict, "verdict PASS on safety behavior");
    }
}

}
