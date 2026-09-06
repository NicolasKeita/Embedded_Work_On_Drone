/*
Filename: Tests/Hil/HilTests-Data.cpp
Description: HIL data-integrity tests : sensor packet originates from the simulated sensor
chain (never ground truth), actuator packets drive the aircraft, telemetry matches the
data path, and truth/sensor streams stay distinguishable.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import HilConfig;
import HilRunner;
import HilRunnerContext;
import HilScenarios;
import SilFaultScenario;
import TestHarness;

namespace sim::test::hil {

namespace {
    sim::hil::HilConfig noise_config(std::float64_t noise, std::float64_t duration_s)
    {
        sim::hil::HilConfig config = sim::hil::hil_base_config();
        config.scenario_id = "NOM-001_StationKeeping";
        config.duration_s = duration_s;
        config.real_time_pacing = false;
        config.clock_kind = sim::hil::ClockKind::Fast;
        config.sensor_noise_stddev = noise;
        return config;
    }

    std::expected<sim::hil::HilRunOutput, sim::hil::HilError> run(const sim::hil::HilConfig& cfg)
    {
        sim::hil::HilRunner runner{cfg};
        const std::array<sim::sil::FaultScenario, 1> scenarios{sim::sil::FaultScenario{}};
        return runner.run(scenarios);
    }
}

void run_data_tests(sim::test::TestHarness& runner)
{
    runner.set_context("DATA");
    const sim::hil::HilConfig perfect = noise_config(0.0, 2.0);
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> perfect_outcome = run(perfect);
    runner.check(perfect_outcome.has_value(), "perfect-sensor run executed");
    if (!perfect_outcome.has_value()) {
        return;
    }
    const sim::hil::HilRunOutput& output = *perfect_outcome;

    runner.check(!output.ground_truth.empty() && !output.telemetry.empty(),
                 "both truth and sensor streams recorded");
    runner.check(output.ground_truth.size() == output.telemetry.size(),
                 "truth and sensor streams sampled in lockstep");

    bool sensor_matches_truth = true;
    for (std::size_t i = 0; i < output.telemetry.size() && i < output.ground_truth.size(); ++i) {
        if (std::abs(output.telemetry[i].z - output.ground_truth[i].z) > 1.0e-4) {
            sensor_matches_truth = false;
            break;
        }
    }
    runner.check(sensor_matches_truth, "with noise 0 the sensor stream equals the truth stream");

    const double takeoff_z = output.ground_truth.back().z;
    runner.check(takeoff_z > 0.0, "aircraft state evolved from FC actuator commands (took off, z>0)");

    const sim::hil::HilConfig noisy = noise_config(0.25, 3.0);
    const std::expected<sim::hil::HilRunOutput, sim::hil::HilError> noisy_outcome = run(noisy);
    runner.check(noisy_outcome.has_value(), "noisy-sensor run executed");
    if (!noisy_outcome.has_value()) {
        return;
    }
    const sim::hil::HilRunOutput& noisy_output = *noisy_outcome;
    bool streams_distinguishable = false;
    for (std::size_t i = 0; i < noisy_output.telemetry.size() && i < noisy_output.ground_truth.size(); ++i) {
        if (std::abs(noisy_output.telemetry[i].z - noisy_output.ground_truth[i].z) > 1.0e-4) {
            streams_distinguishable = true;
            break;
        }
    }
    runner.check(streams_distinguishable, "with noise>0 the truth and sensor streams are distinguishable");
}

}
