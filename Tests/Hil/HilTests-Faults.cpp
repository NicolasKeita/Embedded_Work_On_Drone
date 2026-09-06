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
    std::expected<sim::hil::HilRunOutput, sim::hil::HilError> run_scenario(std::string_view id,
                                                                           std::float64_t duration_s)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        sim::hil::HilConfig                config = record->config;

        config.scenario_id = id;
        config.duration_s = duration_s;
        config.real_time_pacing = false;
        config.clock_kind = sim::hil::ClockKind::Fast;
        sim::hil::HilRunner runner{config};
        const std::array<sim::sil::FaultScenario, 1> scenarios{record->fault};
        return runner.run(scenarios);
    }

    /* Abort family: the fault must be detected and drive the mission to SAFE_MODE/ABORTED. */
    void check_abort_scenario(sim::test::TestHarness&  runner,
                              std::string_view         id,
                              sim::safety::FaultDomain domain)
    {
        runner.set_context(id);
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario(id, 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) {
            return;
        }
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "fault detected");
        runner.check(r.first_fault_domain == domain, "fault classified as expected");
        runner.check(r.safe_mode_reached, "SAFE_MODE engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED");
        runner.check(r.test_verdict, "verdict PASS on safety behavior");
    }

    /* Compensated family: the fault must be detected and compensated, the mission pursues. */
    void check_compensated_scenario(sim::test::TestHarness&  runner,
                                    std::string_view         id,
                                    sim::safety::FaultDomain domain)
    {
        runner.set_context(id);
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario(id, 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) {
            return;
        }
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "fault detected");
        runner.check(r.first_fault_domain == domain, "fault classified as expected");
        runner.check(r.degraded_reached, "HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "COMPENSATED mode engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
        runner.check(r.test_verdict, "verdict PASS on degraded handling");
    }
}

void run_fault_tests(sim::test::TestHarness& runner)
{
    check_abort_scenario(runner, "FAULT_INJECTOR-001", sim::safety::FaultDomain::FC1Heartbeat);
    check_abort_scenario(runner, "FAULT_INJECTOR-002", sim::safety::FaultDomain::Communication);
    check_compensated_scenario(runner, "FAULT_INJECTOR-003", sim::safety::FaultDomain::Sensor);
    check_compensated_scenario(runner, "FAULT_INJECTOR-004", sim::safety::FaultDomain::Actuator);
    check_abort_scenario(runner, "MONTE_CARLO_FAULT_INJECTOR-001", sim::safety::FaultDomain::FC1Heartbeat);
}

}
