/*
Filename: Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Support.cpp
Description: Shared helpers of the telemetry test suite (determinism, sensor path and command divergence).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import SilRunner;
import SilTelemetry;
import TestHarness;

namespace sim::test::sil {

using sim::sil::SilConfig;
using sim::sil::SilRunOutput;
using sim::sil::TelemetrySample;
using sim::sil::TrueStateSample;

/*
True when two telemetry streams carry identical samples (deterministic run).
*/
bool same_telemetry(const std::vector<TelemetrySample>& a, const std::vector<TelemetrySample>& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t index = 0; index < a.size(); ++index) {
        const bool identical = a[index].time == b[index].time && a[index].altitude_m == b[index].altitude_m
                               && a[index].commanded_rpm == b[index].commanded_rpm
                               && a[index].actual_rpm == b[index].actual_rpm;
        if (!identical) {
            return false;
        }
    }
    return true;
}

/*
Checks that the controller commands match the nominal run before the fault
window and diverge inside it: the FC consumed the sensor-path data, not the
ground truth.
*/
void check_commands_diverge(TestHarness&        runner,
                            const SilRunOutput& nominal,
                            const SilRunOutput& faulted,
                            double              fault_start,
                            double              fault_end)
{
    bool pre_commands_identical = true;
    bool post_commands_diverge = false;

    for (std::size_t index = 0;
         index < faulted.telemetry.size() && index < nominal.telemetry.size(); ++index) {
        const TelemetrySample& n = nominal.telemetry[index];
        const TelemetrySample& f = faulted.telemetry[index];

        if (f.time < fault_start) {
            pre_commands_identical =
                pre_commands_identical && n.commanded_rpm == f.commanded_rpm && n.actual_rpm == f.actual_rpm;
        }
        else if (f.time < fault_end) {
            post_commands_diverge = post_commands_diverge || n.commanded_rpm != f.commanded_rpm;
        }
    }
    runner.check(pre_commands_identical, "TELE : commandes identiques au nominal avant la faulte");
    runner.check(post_commands_diverge, "TELE : le FC consomme la mesure du chemin capteur");
}

/*
Checks the truth and sensor streams of the faulted run: ground truth stays
finite (and physical before the fault) while the sensor path diverges from it
during the corruption window.
*/
void check_stream_separation(TestHarness&        runner,
                             const SilRunOutput& output,
                             const SilConfig&    config,
                             double              fault_start,
                             double              fault_end)
{
    const std::vector<TelemetrySample>& samples = output.telemetry;
    const std::vector<TrueStateSample>& truth = output.ground_truth;

    runner.check(samples.size() == truth.size(), "TELE-011 : flux telemetrie/ground truth alignes");

    bool truth_finite = true;
    bool truth_valid_before_fault = true;
    bool pre_fault_identical = true;
    bool sensor_diverged = false;
    for (std::size_t index = 0; index < samples.size(); ++index) {
        truth_finite = truth_finite && std::isfinite(truth[index].z);
        if (samples[index].time < fault_start) {
            truth_valid_before_fault = truth_valid_before_fault && truth[index].z >= 0.0
                                       && truth[index].z <= config.sensor_limits.max_altitude_m;
            pre_fault_identical = pre_fault_identical && samples[index].altitude_m == truth[index].z;
        }
        else if (samples[index].time < fault_end) {
            sensor_diverged = sensor_diverged
                              || std::abs(samples[index].altitude_m - truth[index].z) > 1.0e-6;
        }
    }
    runner.check(truth_finite, "TELE-011 : ground truth toujours finie (jamais corrompue)");
    runner.check(truth_valid_before_fault, "TELE-011 : ground truth dans les limites physiques avant la faulte");
    runner.check(pre_fault_identical, "TELE-011 : sans corruption capteur et verite coincident");
    runner.check(sensor_diverged, "TELE-011 : telemetrie capteur ecartee de la verite pendant la faulte");
}

}
