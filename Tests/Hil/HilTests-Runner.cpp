/*
Filename: Tests/Hil/HilTests-Runner.cpp
Description: HIL runner tests : step count, real-time pacing, deadline detection,
telemetry cadence and event ordering.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import Aircraft;
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
    sim::hil::HilConfig test_config(std::string_view id, std::float64_t duration_s)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        sim::hil::HilConfig                config = record ? record->config : sim::hil::hil_base_config();

        config.scenario_id = id;
        config.duration_s = duration_s;
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

    /* Checks wind timing, direction, ground gating and unchanged calm dynamics. */
    void test_wind(sim::test::TestHarness& runner)
    {
        runner.set_context("WIND");
        const sim::hil::HilConfig steady = test_config("WIND-001", 30.0);
        const sim::hil::HilConfig gust = test_config("WIND-002", 30.0);
        runner.check(steady.wind_y_mps == 4.0, "steady wind catalog configuration");
        runner.check(sim::hil::wind_factor(steady, 7.99) == 0.0, "calm before wind");
        runner.check(sim::hil::wind_factor(steady, 8.0) == 1.0, "wind starts at 8 s");
        runner.check(sim::hil::wind_factor(steady, 24.0) == 0.0, "wind stops at 24 s");
        runner.check(sim::hil::wind_factor(gust, 8.0) == 0.0, "gust starts smoothly");
        runner.check(sim::hil::wind_factor(gust, 10.0) == 1.0, "gust reaches peak");
        runner.check(sim::hil::wind_factor(gust, 12.0) == 0.0, "gust repeats");
        Aircraft calm;
        Aircraft windy;
        windy.set_wind(0.0, 4.0);
        windy.update(0.01);
        runner.check(windy.state().vy == 0.0, "wind does not slide grounded aircraft");
        const ControlCommand climb{.wing_rpm = calm.hover_rpm() * 1.1};
        calm.set_command(climb);
        windy.set_command(climb);
        for (std::uint32_t step = 0; step < 100; ++step) {
            calm.update(0.01);
            windy.update(0.01);
        }
        runner.check(windy.state().y > calm.state().y, "wind physically displaces airborne aircraft");
        runner.check(windy.state().x == calm.state().x, "crosswind preserves perpendicular axis");
        runner.check(windy.state().z == calm.state().z, "horizontal wind preserves vertical dynamics");
    }

    void test_nominal_run(sim::test::TestHarness& runner)
    {
        runner.set_context("NOMINAL-001");
        const sim::hil::HilConfig cfg = test_config("NOMINAL-001", 30.0);
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find("NOMINAL-001");
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
    test_wind(runner);
    test_nominal_run(runner);
    test_timing(runner);
    test_realtime(runner);
}

}
