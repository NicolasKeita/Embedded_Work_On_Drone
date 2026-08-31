/*
Filename: Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-FaultPath.cpp
Description: Sensor-fault data path test: ground truth, sensor telemetry, FC consumption and events.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import SilEvents;
import SilRunner;
import SilTelemetry;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SensorCorruptionMode;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilRunOutput;
using sim::sil::SILRunner;
using sim::sil::TelemetrySample;
using sim::sil::TrueStateSample;

void check_commands_diverge(TestHarness& runner, const SilRunOutput& nominal,
                            const SilRunOutput& faulted, double fault_start, double fault_end);

namespace {

constexpr double kFaultStart = 20.0, kFaultEnd = 30.0;

/*
Checks the truth and sensor streams of the faulted run: ground truth stays
finite (and physical before the fault) while the sensor path diverges from it
during the corruption window.
*/
void check_stream_separation(TestHarness& runner, const SilRunOutput& output, const SilConfig& config)
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
        if (samples[index].time < kFaultStart) {
            truth_valid_before_fault = truth_valid_before_fault && truth[index].z >= 0.0
                                       && truth[index].z <= config.sensor_limits.max_altitude_m;
            pre_fault_identical = pre_fault_identical && samples[index].altitude_m == truth[index].z;
        }
        else if (samples[index].time < kFaultEnd) {
            sensor_diverged = sensor_diverged
                              || std::abs(samples[index].altitude_m - truth[index].z) > 1.0e-6;
        }
    }
    runner.check(truth_finite, "TELE-011 : ground truth toujours finie (jamais corrompue)");
    runner.check(truth_valid_before_fault,
                 "TELE-011 : ground truth dans les limites physiques avant la faulte");
    runner.check(pre_fault_identical, "TELE-011 : sans corruption capteur et verite coincident");
    runner.check(sensor_diverged, "TELE-011 : telemetrie capteur ecartee de la verite pendant la faulte");
}

}

/*
TELE-011: a sensor fault injected in the environment reaches the FC through the
sensor path only: the ground truth stays valid, the sensor telemetry records the
faulty measurement, the fault events carry the parameters and the controller
reacts to the corrupted data.
*/
void telemetry_sensor_fault_path_test(TestHarness& runner)
{
    const SilConfig config{.duration_s = 35.0};

    const FaultScenario fault{.start_time = kFaultStart,
                              .duration = kFaultEnd - kFaultStart,
                              .fault_type = FaultType::SensorFault,
                              .parameters = {.corruption = SensorCorruptionMode::ExtremeNoise}};

    const std::array<FaultScenario, 1> nominal_scenarios{FaultScenario{}};
    const std::array<FaultScenario, 1> fault_scenarios{fault};
    const std::expected<SilRunOutput, SilError> nominal = SILRunner{config}.run(nominal_scenarios);
    const std::expected<SilRunOutput, SilError> faulted = SILRunner{config}.run(fault_scenarios);

    if (!nominal.has_value() || !faulted.has_value()) {
        runner.check(false, "TELE-011 : moteur SIL en echec");
        return;
    }

    check_stream_separation(runner, faulted.value(), config);
    check_commands_diverge(runner, nominal.value(), faulted.value(), kFaultStart, kFaultEnd);

    const SilEvent* injected = find_first(faulted.value().events, SilEventType::FaultInjected);
    const SilEvent* sensor_fault = find_first(faulted.value().events, SilEventType::SensorFault);

    runner.check(injected != nullptr, "TELE-011 : evenement FAULT_INJECTED present");
    runner.check(sensor_fault != nullptr, "TELE-011 : evenement SENSOR_FAULT present");
    if (injected == nullptr) {
        return;
    }
    runner.check(injected->detail == sim::sil::fault_type_name(FaultType::SensorFault)
                     && injected->reason == sim::sil::corruption_mode_name(SensorCorruptionMode::ExtremeNoise),
                 "TELE-011 : FAULT_INJECTED porte type et mode de corruption");
}

}