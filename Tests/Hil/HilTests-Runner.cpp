/*
Filename: Tests/Hil/HilTests-Runner.cpp
Description: HIL runner tests : step count, real-time pacing, deadline detection,
telemetry cadence and event ordering.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import FlightControllerTypes;
import HilConfig;
import HilRunner;
import HilRunnerContext;
import HilScenarios;
import HilTiming;
import SilFaultScenario;
import TestHarness;

namespace sim::test::hil {

namespace {
    sim::hil::HilConfig fast_config(std::string_view id, std::float64_t duration_s)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        sim::hil::HilConfig                config = record ? record->config : sim::hil::hil_base_config();

        config.scenario_id = id;
        config.duration_s = duration_s;
        config.real_time_pacing = false;
        config.clock_kind = sim::hil::ClockKind::Fast;
        return config;
    }

    bool events_ordered(const sim::hil::HilRunOutput& output)
    {
        for (std::size_t i = 1; i < output.events.size(); ++i) {
            if (output.events[i].sim_time_s + 1.0e-9 < output.events[i - 1].sim_time_s) {
                return false;
            }
        }
        return true;
    }

    void test_nominal_run(sim::test::TestHarness& runner)
    {
        runner.set_context("NOM-001_StationKeeping");
        const sim::hil::HilConfig cfg = fast_config("NOM-001_StationKeeping", 30.0);
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find("NOM-001_StationKeeping");
        const std::array<sim::sil::FaultScenario, 1> scenarios{record->fault};
        sim::hil::HilRunner runner_obj{cfg};
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> outcome = runner_obj.run(scenarios);

        runner.check(outcome.has_value(), "runner executed");
        if (!outcome.has_value()) {
            return;
        }
        const sim::hil::HilRunOutput& output = *outcome;

        const std::uint64_t expected_steps = sim::hil::hil_step_count(cfg);
        runner.check(output.result.timing.steps_executed == expected_steps,
                     "executed all steps (duration / dt, not hard-coded)");
        runner.check(expected_steps == 3000, "30 s / 10 ms = 3000 steps");

        runner.check(output.result.mission_success, "mission completed (COMPLETE)");
        runner.check(output.result.final_state == sim::control::MissionState::COMPLETE, "final mission state COMPLETE");
        runner.check(output.result.timing.deadline_misses == 0, "zero deadline misses");
        runner.check(output.result.test_verdict, "verdict PASS");

        runner.check(!output.telemetry.empty(), "structured telemetry emitted");
        const std::size_t expected_samples = static_cast<std::size_t>(cfg.duration_s * cfg.telemetry_rate_hz);
        runner.check(output.telemetry.size() >= expected_samples / 2,
                     "telemetry cadence near the configured internal rate");
        runner.check(events_ordered(output), "event ordering preserved by sim time");
    }

}

void test_timing(sim::test::TestHarness& runner);
void test_realtime(sim::test::TestHarness& runner);

void run_runner_tests(sim::test::TestHarness& runner)
{
    test_nominal_run(runner);
    test_timing(runner);
    test_realtime(runner);
}

}
