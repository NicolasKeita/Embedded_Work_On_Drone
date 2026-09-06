/*
Filename: Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-FaultPath.cpp
Description: Sensor-fault data path test: ground truth, sensor telemetry, FC consumption and events.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservabilityTelemetry;

import std;

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
using sim::sil::SensorCorruptionMode;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilRunOutput;
using sim::sil::SILRunner;

void check_commands_diverge(TestHarness&        runner,
                            const SilRunOutput& nominal,
                            const SilRunOutput& faulted,
                            std::float64_t      fault_start,
                            std::float64_t      fault_end);
void check_stream_separation(TestHarness&        runner,
                             const SilRunOutput& output,
                             const SilConfig&    config,
                             std::float64_t      fault_start,
                             std::float64_t      fault_end);

constexpr std::float64_t kFaultStart = 20.0;
constexpr std::float64_t kFaultEnd   = 30.0;

/*
TELE-011: a sensor fault injected in the environment reaches the FC through the
sensor path only: the ground truth stays valid, the sensor telemetry records the
faulty measurement, the fault events carry the parameters and the controller
reacts to the corrupted data.
*/
void telemetry_sensor_fault_path_test(TestHarness& runner)
{
    runner.set_context("TELE-011");

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
        runner.check(false, "SIL runner failed");
        return;
    }

    check_stream_separation(runner, faulted.value(), config, kFaultStart, kFaultEnd);
    check_commands_diverge(runner, nominal.value(), faulted.value(), kFaultStart, kFaultEnd);

    const SilEvent* injected = find_first(faulted.value().events, SilEventType::FaultInjected);
    const SilEvent* sensor_fault = find_first(faulted.value().events, SilEventType::SensorFault);

    runner.check(injected != nullptr, "FAULT_INJECTED event present");
    runner.check(sensor_fault != nullptr, "SENSOR_FAULT event present");
    if (injected == nullptr) {
        return;
    }
    runner.check(injected->detail == sim::sil::fault_type_name(FaultType::SensorFault)
                     && injected->reason.find("EXTREME_NOISE") != std::string_view::npos
                     && injected->reason.find("temporarily") != std::string_view::npos,
                 "FAULT_INJECTED carries type, mode and duration");
    runner.check(injected->has_duration
                     && std::abs(injected->duration_s - (kFaultEnd - kFaultStart)) <= 1.0e-9,
                 "FAULT_INJECTED carries the explicit duration of the temporary fault");
}

}
