/*
Filename: Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Support.cpp
Description: Shared helpers of the telemetry test suite (determinism and command divergence).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import SilRunner;
import SilTelemetry;
import TestHarness;

namespace sim::test::sil {

using sim::sil::SilRunOutput;
using sim::sil::TelemetrySample;

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
void check_commands_diverge(TestHarness& runner, const SilRunOutput& nominal,
                            const SilRunOutput& faulted, double fault_start, double fault_end)
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

}