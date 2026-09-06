/*
Filename: Tests/Hil/HilTests-Runner.cpp
Description: HIL runner tests : step count, timestep, real-time pacing, simulation-time
coverage, deadline detection, telemetry cadence and event ordering.

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
        sim::hil::HilConfig config = record ? record->config : sim::hil::hil_base_config();
        config.scenario_id = id;
        config.duration_s = duration_s;
        config.real_time_pacing = false;
        config.clock_kind = sim::hil::ClockKind::Fast;
        return config;
    }

    const sim::sil::FaultScenario& scenario_of(std::string_view id)
    {
        const sim::hil::HilScenarioRecord* record = sim::hil::HilScenarioCatalog::find(id);
        return record->fault;
    }
}

void run_runner_tests(sim::test::TestHarness& runner)
{
    const sim::hil::HilConfig cfg = fast_config("HIL-001", 30.0);
    const std::array<sim::sil::FaultScenario, 1> scenarios{scenario_of("HIL-001")};
    sim::hil::HilRunner runner_obj{cfg};
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> outcome = runner_obj.run(scenarios);

    runner.check(outcome.has_value(), "HIL-001 : runner executed");
    if (!outcome.has_value()) {
        return;
    }
    const sim::hil::HilRunOutput& output = *outcome;

    const std::uint64_t expected_steps = sim::hil::hil_step_count(cfg);
    runner.check(output.result.timing.steps_executed == expected_steps,
                 "HIL-001 : executed all steps (duration / dt, not hard-coded)");
    runner.check(expected_steps == 3000, "HIL-001 : 30 s / 10 ms = 3000 steps");

    runner.check(output.result.mission_success, "HIL-001 : mission completed (COMPLETE)");
    runner.check(output.result.final_state == sim::control::MissionState::COMPLETE,
                 "HIL-001 : final mission state COMPLETE");
    runner.check(output.result.timing.deadline_misses == 0, "HIL-001 : zero deadline misses");
    runner.check(output.result.test_verdict, "HIL-001 : verdict PASS");

    runner.check(!output.telemetry.empty(), "HIL-001 : structured telemetry emitted");
    const std::size_t expected_samples = static_cast<std::size_t>(cfg.duration_s * cfg.telemetry_rate_hz);
    runner.check(output.telemetry.size() >= expected_samples / 2,
                 "HIL-001 : telemetry cadence near the configured internal rate");

    bool ordered = true;
    for (std::size_t i = 1; i < output.events.size(); ++i) {
        if (output.events[i].sim_time_s + 1.0e-9 < output.events[i - 1].sim_time_s) {
            ordered = false;
            break;
        }
    }
    runner.check(ordered, "HIL-001 : event ordering preserved by sim time");

    sim::hil::HilStepTiming timing{};
    timing.scheduled_us = 0;
    timing.step_completion_us = 15000;
    runner.check(timing.deadline_missed(10000), "timing : deadline miss detected when over budget");
    timing.step_completion_us = 9000;
    runner.check(!timing.deadline_missed(10000), "timing : no miss when within budget");

    sim::hil::HilTimingStats stats{};
    sim::hil::HilStepTiming miss{};
    miss.scheduled_us = 0;
    miss.actual_start_us = 0;
    miss.step_completion_us = 12000;
    stats.record(miss, 10000);
    runner.check(stats.deadline_misses == 1, "timing : HilTimingStats counts a deadline miss");

    sim::hil::HilConfig realtime_cfg = sim::hil::hil_base_config();
    realtime_cfg.scenario_id = "HIL-001";
    realtime_cfg.duration_s = 0.10;
    realtime_cfg.dt_s = 0.01;
    realtime_cfg.real_time_pacing = true;
    realtime_cfg.clock_kind = sim::hil::ClockKind::Monotonic;
    const std::array<sim::sil::FaultScenario, 1> rs{sim::hil::HilScenarioCatalog::find("HIL-001")->fault};
    const auto t0 = std::chrono::steady_clock::now();
    sim::hil::HilRunner rt_runner{realtime_cfg};
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> rt = rt_runner.run(rs);
    const auto t1 = std::chrono::steady_clock::now();
    const double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
    runner.check(rt.has_value(), "HIL: real-time run executed");
    runner.check(elapsed_s >= 0.09, "HIL: real-time pacing active (wall clock elapses ~ duration)");
}

}
