/*
Filename: Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Sampling.cpp
Description: Telemetry sampling tests: rate, simulation timestamps, determinism and logging neutrality.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import Aircraft;
import SilEvents;
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
    const SilConfig config{.duration_s = 10.0, .telemetry_rate_hz = 20.0};
    const FaultScenario scenario{};
    const std::expected<SilRunOutput, SilError> first = run_traced(config, scenario);
    const std::expected<SilRunOutput, SilError> second = run_traced(config, scenario);

    if (!first.has_value() || !second.has_value()) {
        runner.check(false, "TELE-010 : moteur SIL en echec");
        return;
    }

    const std::vector<TelemetrySample>& samples = first.value().telemetry;
    const std::vector<TrueStateSample>& truth = first.value().ground_truth;

    runner.check(samples.size() == 201, "TELE-010 : 201 echantillons a 20 Hz sur 10 s");
    runner.check(truth.size() == samples.size(), "TELE-010 : flux ground truth aligne sur la telemetrie");

    bool timestamps_ok = true;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        timestamps_ok = timestamps_ok
                        && std::abs(samples[index].time - 0.05 * static_cast<std::float64_t>(index)) < 1.0e-6;
    }
    runner.check(timestamps_ok, "TELE-010 : horodatage regulier en temps de simulation");
    runner.check(same_telemetry(samples, second.value().telemetry),
                 "TELE-010 : echantillonnage deterministe entre deux runs");

    if (samples.empty()) {
        return;
    }
    const TelemetrySample& last = samples.back();
    runner.check(std::abs(last.target_z - config.target.z) < 1.0e-9,
                 "TELE-010 : cible de consigne reportee dans la telemetrie");
    runner.check(last.mission_state <= 5 && last.safety_state <= 2, "TELE-010 : etats mission/surete renseignes");
}

/*
TELE-013: the telemetry rate never alters the simulation outcome, only the
number of recorded samples (observational neutrality of telemetry logging).
*/
void telemetry_rate_neutrality_test(TestHarness& runner)
{
    const FaultScenario scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const SilConfig slow{.duration_s = 35.0, .telemetry_rate_hz = 5.0};
    const SilConfig standard{.duration_s = 35.0, .telemetry_rate_hz = 20.0};
    const SilConfig fast{.duration_s = 35.0, .telemetry_rate_hz = 50.0};
    const std::expected<SilRunOutput, SilError> slow_out = run_traced(slow, scenario);
    const std::expected<SilRunOutput, SilError> standard_out = run_traced(standard, scenario);
    const std::expected<SilRunOutput, SilError> fast_out = run_traced(fast, scenario);

    if (!slow_out.has_value() || !standard_out.has_value() || !fast_out.has_value()) {
        runner.check(false, "TELE-013 : moteur SIL en echec");
        return;
    }

    const SimulationResult& a = slow_out.value().result;
    const SimulationResult& b = standard_out.value().result;
    const SimulationResult& c = fast_out.value().result;

    runner.check(a.mission_success == b.mission_success && b.mission_success == c.mission_success,
                 "TELE-013 : succes mission independant de la cadence");
    runner.check(a.final_state == b.final_state && b.final_state == c.final_state,
                 "TELE-013 : etat terminal independant de la cadence");
    runner.check(a.detection_latency == b.detection_latency && b.detection_latency == c.detection_latency,
                 "TELE-013 : latence de detection independante de la cadence");
    runner.check(a.max_altitude_error_m == b.max_altitude_error_m
                     && b.max_altitude_error_m == c.max_altitude_error_m,
                 "TELE-013 : metriques aircraft independantes de la cadence");
    runner.check(a.comms.sent == b.comms.sent && b.comms.sent == c.comms.sent,
                 "TELE-013 : trafic heartbeat independant de la cadence");
    runner.check(slow_out.value().telemetry.size() < standard_out.value().telemetry.size()
                     && standard_out.value().telemetry.size() < fast_out.value().telemetry.size(),
                 "TELE-013 : nombre d'echantillons suit la cadence");
}

}
