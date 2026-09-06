/*
Filename: Tests/Hil/HilTests-Faults.cpp
Description: HIL fault tests : FC1 failure, communication loss, sensor fault and actuator
degradation, asserting the fault is injected through the HIL data path, detected
naturally by the reused safety/health core and handled per the expected safety behaviour.

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
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("HIL-002", 12.0);
        runner.check(o.has_value(), "HIL-002 : run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "HIL-002 : FC1 failure detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::FC1Heartbeat,
                     "HIL-002 : fault classified as FC1 heartbeat");
        runner.check(r.safe_mode_reached, "HIL-002 : SAFE_MODE engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "HIL-002 : mission ABORTED");
        runner.check(r.test_verdict, "HIL-002 : verdict PASS on safety behaviour");
    }

    {
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("HIL-003", 12.0);
        runner.check(o.has_value(), "HIL-003 : run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "HIL-003 : communication loss detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Communication,
                     "HIL-003 : fault classified as communication");
        runner.check(r.safe_mode_reached, "HIL-003 : safety reaction engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "HIL-003 : mission ABORTED");
        runner.check(r.test_verdict, "HIL-003 : verdict PASS on safety behaviour");
    }

    {
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("HIL-004", 12.0);
        runner.check(o.has_value(), "HIL-004 : run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "HIL-004 : sensor fault detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Sensor,
                     "HIL-004 : fault classified as sensor");
        runner.check(r.degraded_reached, "HIL-004 : HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "HIL-004 : COMPENSATED mode engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "HIL-004 : mission not aborted");
        runner.check(r.test_verdict, "HIL-004 : verdict PASS on degraded handling");
    }

    {
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario("HIL-005", 12.0);
        runner.check(o.has_value(), "HIL-005 : run executed");
        if (!o) return;
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "HIL-005 : actuator degradation detected");
        runner.check(r.first_fault_domain == sim::safety::FaultDomain::Actuator,
                     "HIL-005 : fault classified as actuator");
        runner.check(r.degraded_reached, "HIL-005 : HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "HIL-005 : thrust compensation engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "HIL-005 : mission not aborted");
        runner.check(r.test_verdict, "HIL-005 : verdict PASS on compensation");
    }
}

}
