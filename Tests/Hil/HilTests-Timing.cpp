/*
Filename: Tests/Hil/HilTests-Timing.cpp
Description: HIL runner timing tests : deadline-miss detection, timing statistics and
real-time wall-clock pacing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import HilConfig;
import HilRunner;
import HilRunnerContext;
import HilScenarios;
import HilTiming;
import SilFaultScenario;
import TestHarness;

namespace sim::test::hil {

    void test_timing(sim::test::TestHarness& runner)
    {
        runner.set_context("TIMING");
        sim::hil::HilStepTiming timing{.scheduled_us = 0, .step_completion_us = 15000};
        runner.check(timing.deadline_missed(10000), "deadline miss detected when over budget");
        timing.step_completion_us = 9000;
        runner.check(!timing.deadline_missed(10000), "no miss when within budget");

        sim::hil::HilTimingStats stats{};
        sim::hil::HilStepTiming miss{.scheduled_us = 0, .actual_start_us = 0, .step_completion_us = 12000};
        stats.record(miss, 10000);
        runner.check(stats.deadline_misses == 1, "HilTimingStats counts a deadline miss");
    }

    void test_realtime(sim::test::TestHarness& runner)
    {
        runner.set_context("REALTIME");
        sim::hil::HilConfig realtime_cfg = sim::hil::hil_base_config();
        realtime_cfg.scenario_id = "NOMINAL-001";
        realtime_cfg.duration_s = 0.10;
        realtime_cfg.dt_s = 0.01;
        realtime_cfg.real_time_pacing = true;
        realtime_cfg.clock_kind = sim::hil::ClockKind::Monotonic;
        const std::array<sim::sil::FaultScenario, 1> rs{ sim::hil::HilScenarioCatalog::find("NOMINAL-001")->fault};
        const auto t0 = std::chrono::steady_clock::now();
        sim::hil::HilRunner rt_runner{realtime_cfg};
        const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> rt = rt_runner.run(rs);
        const auto t1 = std::chrono::steady_clock::now();
        const double elapsed_s = std::chrono::duration<double>(t1 - t0).count();
        runner.check(rt.has_value(), "HIL: real-time run executed");
        runner.check(elapsed_s >= 0.09, "HIL: real-time pacing active (wall clock elapses ~ duration)");
    }

}
