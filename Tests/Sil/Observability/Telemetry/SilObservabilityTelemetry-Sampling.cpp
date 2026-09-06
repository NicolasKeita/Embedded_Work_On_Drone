/*
Filename: Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Sampling.cpp
Description: Telemetry sampling tests: rate, simulation timestamps, determinism and logging neutrality.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservabilityTelemetry;

import std;

import Aircraft;
import SilEvents;
import SilObservability;
import SilRunner;
import SilTelemetry;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilRunOutput;
using sim::sil::SILRunner;
using sim::sil::SimulationResult;
using sim::sil::TelemetrySample;
using sim::sil::TrueStateSample;

bool same_telemetry(const std::vector<TelemetrySample>& a, const std::vector<TelemetrySample>& b);

/*
TELE-010: telemetry is sampled at the configured rate in simulation time, the
ground-truth stream mirrors it sample for sample, control context fields are
populated and a second identical run is bit-for-bit identical.
*/
void telemetry_sampling_test(TestHarness& runner)
{
    runner.set_context("TELE-010");

    const SilConfig                             config{.duration_s = 10.0, .telemetry_rate_hz = 20.0};
    const FaultScenario                         scenario{};
    const std::expected<SilRunOutput, SilError> first = run_traced(config, scenario);
    const std::expected<SilRunOutput, SilError> second = run_traced(config, scenario);

    if (!first.has_value() || !second.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const std::vector<TelemetrySample>& samples = first.value().telemetry;
    const std::vector<TrueStateSample>& truth = first.value().ground_truth;

    runner.check(samples.size() == 201, "201 samples at 20 Hz over 10 s");
    runner.check(truth.size() == samples.size(), "ground-truth stream aligned with telemetry");

    bool timestamps_ok = true;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        timestamps_ok = timestamps_ok
                        && std::abs(samples[index].time - 0.05 * static_cast<std::float64_t>(index)) < 1.0e-6;
    }
    runner.check(timestamps_ok, "regular timestamps in simulation time");
    runner.check(same_telemetry(samples, second.value().telemetry),
                 "deterministic sampling across two runs");

    if (samples.empty()) {
        return;
    }
    const TelemetrySample& last = samples.back();
    runner.check(std::abs(last.target_z - config.target.z) < 1.0e-9,
                 "setpoint target carried into telemetry");
    runner.check(last.mission_state <= 5 && last.safety_state <= 2, "mission/safety states populated");
}

/*
TELE-013: the telemetry rate never alters the simulation outcome, only the
number of recorded samples (observational neutrality of telemetry logging).
*/
void telemetry_rate_neutrality_test(TestHarness& runner)
{
    runner.set_context("TELE-013");

    const FaultScenario                         scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const SilConfig                             slow{.duration_s = 35.0, .telemetry_rate_hz = 5.0};
    const SilConfig                             standard{.duration_s = 35.0, .telemetry_rate_hz = 20.0};
    const SilConfig                             fast{.duration_s = 35.0, .telemetry_rate_hz = 50.0};
    const std::expected<SilRunOutput, SilError> slow_out = run_traced(slow, scenario);
    const std::expected<SilRunOutput, SilError> standard_out = run_traced(standard, scenario);
    const std::expected<SilRunOutput, SilError> fast_out = run_traced(fast, scenario);

    if (!slow_out.has_value() || !standard_out.has_value() || !fast_out.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    const SimulationResult& a = slow_out.value().result;
    const SimulationResult& b = standard_out.value().result;
    const SimulationResult& c = fast_out.value().result;

    runner.check(a.mission_success == b.mission_success && b.mission_success == c.mission_success,
                 "mission success independent of the rate");
    runner.check(a.final_state == b.final_state && b.final_state == c.final_state,
                 "terminal state independent of the rate");
    runner.check(a.detection_latency == b.detection_latency && b.detection_latency == c.detection_latency,
                 "detection latency independent of the rate");
    runner.check(a.max_altitude_error_m == b.max_altitude_error_m
                     && b.max_altitude_error_m == c.max_altitude_error_m,
                 "aircraft metrics independent of the rate");
    runner.check(a.comms.sent == b.comms.sent && b.comms.sent == c.comms.sent,
                 "heartbeat traffic independent of the rate");
    runner.check(slow_out.value().telemetry.size() < standard_out.value().telemetry.size()
                     && standard_out.value().telemetry.size() < fast_out.value().telemetry.size(),
                 "number of samples follows the rate");
}

}
