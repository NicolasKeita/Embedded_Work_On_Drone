/*
Filename: Tests/Hil/HilTests-Faults.cpp
Description: HIL fault tests for FC1 and altitude-sensor failures, asserting detection
and safety handling through the HIL data path.

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
import HilTelemetry;
import SilFaultScenario;
import TestHarness;

namespace sim::test::hil {

namespace {
    /* Verifies that a new HIL run gets a complete initial-heartbeat grace period. */
    void test_supervision_rearm(sim::test::TestHarness& runner)
    {
        runner.set_context("FC2 supervision rearm");
        constexpr std::float64_t reset_time_s = 10.0;
        constexpr std::float64_t initial_timeout_s = 2.0;
        sim::safety::LinkSupervision supervision{
            .link_up = true,
            .last_heartbeat_time = -1.0,
            .monitoring_started_time = 0.0,
        };
        sim::safety::HealthMonitor monitor{sim::safety::HealthMonitorConfig{
            .initial_heartbeat_timeout_s = initial_timeout_s,
        }};

        const sim::safety::HealthReport stale_report =
            monitor.evaluate_link(reset_time_s, supervision);
        runner.check(stale_report.state == sim::safety::HealthState::SAFE,
                     "stale initial-heartbeat window has expired");

        monitor = sim::safety::HealthMonitor{sim::safety::HealthMonitorConfig{
            .initial_heartbeat_timeout_s = initial_timeout_s,
        }};
        sim::safety::rearm_link_supervision(supervision, reset_time_s);

        const sim::safety::HealthReport grace_report =
            monitor.evaluate_link(reset_time_s + initial_timeout_s, supervision);
        runner.check(grace_report.state == sim::safety::HealthState::HEALTHY,
                     "rearm grants the complete initial-heartbeat window");

        const sim::safety::HealthReport expired_report =
            monitor.evaluate_link(reset_time_s + initial_timeout_s + 0.01, supervision);
        runner.check(expired_report.state == sim::safety::HealthState::SAFE,
                     "heartbeat timeout is raised after the rearmed window");
        runner.check(expired_report.flag(sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT).raised,
                     "rearmed expiry is classified as FC1 heartbeat timeout");
    }

    std::expected<sim::hil::HilRunOutput, sim::hil::HilError> run_scenario(std::string_view id,
                                                                           std::float64_t duration_s)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        sim::hil::HilConfig                config = record->config;

        config.scenario_id = id;
        config.duration_s = duration_s;
        sim::hil::HilRunner runner{config};
        const std::array<sim::sil::FaultScenario, 1> scenarios{record->fault};
        return runner.run(scenarios);
    }

    /* Abort family: the fault must be detected and drive the mission to SAFE_MODE/ABORTED. */
    void check_abort_scenario(sim::test::TestHarness&     runner,
                              std::string_view            id,
                              sim::safety::DetectionEvent domain)
    {
        runner.set_context(id);
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario(id, 100.0);
        runner.check(o.has_value(), "run executed");
        if (!o) {
            return;
        }
        const sim::hil::HilResult& r = (*o).result;
        const auto before_fault = std::ranges::find_if(o->telemetry.rbegin(), o->telemetry.rend(),
            [](const sim::hil::HilSensorSample& sample) { return sample.time_s < 70.0; });
        runner.check(before_fault != o->telemetry.rend()
                         && std::abs(before_fault->z - 30.0) <= 0.5
                         && before_fault->mission_state == static_cast<std::uint8_t>(sim::control::MissionState::STATION_KEEPING),
                     "station keeping at 30 m before FC1 failure");
        runner.check(r.fault_detected, "fault detected");
        runner.check(r.first_detection_event == domain, "fault classified as expected");
        runner.check(r.safe_mode_reached, "SAFE_MODE engaged");
        runner.check(r.final_state == sim::control::MissionState::ABORTED, "mission ABORTED");
        runner.check(r.test_verdict, "verdict PASS on safety behavior");
    }

    /* Compensated family: the fault must be detected and compensated, the mission pursues. */
    void check_compensated_scenario(sim::test::TestHarness&     runner,
                                    std::string_view            id,
                                    sim::safety::DetectionEvent domain)
    {
        runner.set_context(id);
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> o = run_scenario(id, 12.0);
        runner.check(o.has_value(), "run executed");
        if (!o) {
            return;
        }
        const sim::hil::HilResult& r = (*o).result;
        runner.check(r.fault_detected, "fault detected");
        runner.check(r.first_detection_event == domain, "fault classified as expected");
        runner.check(r.degraded_reached, "HealthMonitor went DEGRADED");
        runner.check(r.compensated_reached, "COMPENSATED mode engaged");
        runner.check(r.final_state != sim::control::MissionState::ABORTED, "mission not aborted");
        runner.check(r.test_verdict, "verdict PASS on degraded handling");
    }
}

void run_fault_tests(sim::test::TestHarness& runner)
{
    test_supervision_rearm(runner);
    check_abort_scenario(runner, "FAULT_INJECTOR-001", sim::safety::DetectionEvent::FC1_HEARTBEAT_TIMEOUT);
    check_compensated_scenario(runner, "FAULT_INJECTOR-003", sim::safety::DetectionEvent::SENSOR_VALIDATION_FAILED);
}

}
