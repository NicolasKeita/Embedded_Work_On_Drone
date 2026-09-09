/*
Filename: Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Hold.cpp
Description: Hold-last-valid and fault clearing telemetry tests with an out-of-range altitude fault.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservabilityTelemetry;

import std;

import Aircraft;
import HealthMonitor;
import SilEvents;
import SilObservability;
import SilRunner;
import SilTelemetry;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::safety::DetectionEvent;
using sim::sil::FaultScenario;
using sim::sil::FailureMode;
using sim::sil::SensorCorruptionMode;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilRunOutput;
using sim::sil::SILRunner;
using sim::sil::TelemetrySample;
using sim::sil::TrueStateSample;

void check_commands_diverge(TestHarness&        runner,
                            const SilRunOutput& nominal,
                            const SilRunOutput& faulted,
                            std::float64_t      fault_start,
                            std::float64_t      fault_end);

namespace {

constexpr std::float64_t kFaultStart = 20.0;
constexpr std::float64_t kFaultEnd = 30.0;

/*
Checks the fault window streams: the forced 99999 m altitude is recorded in the
sensor telemetry while the ground truth stays physical.
*/
void check_fault_window(TestHarness& runner, const SilRunOutput& output, const SilConfig& config)
{
    bool faulty_recorded = false;
    bool truth_stays_physical = true;

    for (std::size_t index = 0; index < output.telemetry.size(); ++index) {
        const TelemetrySample& sample = output.telemetry[index];
        const TrueStateSample& truth = output.ground_truth[index];

        if (sample.time >= kFaultStart && sample.time < kFaultEnd) {
            faulty_recorded = faulty_recorded || sample.altitude_m == 99999.0;
            truth_stays_physical = truth_stays_physical && std::isfinite(truth.z) && truth.z >= 0.0
                                   && truth.z <= config.sensor_limits.max_altitude_m;
        }
    }
    runner.check(faulty_recorded, "99999 m faulty measurement recorded in telemetry");
    runner.check(truth_stays_physical, "ground truth physical during the fault");
    runner.check(output.result.fault_detected
                     && output.result.first_detection_event == DetectionEvent::SENSOR_VALIDATION_FAILED,
                 "sensor fault detected by validation");
}

}

/*
TELE-012: an out-of-range altitude fault is recorded in the sensor telemetry
while the ground truth stays physical, FC1-side validation holds the last valid
measurement and the temporary fault closes with a FAULT_CLEARED event.
*/
void telemetry_sensor_hold_and_clear_test(TestHarness& runner)
{
    runner.set_context("TELE-012");

    const SilConfig config{.duration_s = 40.0};
    const FaultScenario fault{.start_time = kFaultStart,
                              .duration = kFaultEnd - kFaultStart,
                              .failure_mode = FailureMode::INVALID_SENSOR_DATA,
                              .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange,
                                             .corrupted_altitude_m = 99999.0}};
    const std::array<FaultScenario, 1> nominal_scenarios{FaultScenario{}};
    const std::array<FaultScenario, 1> fault_scenarios{fault};
    const std::expected<SilRunOutput, SilError> nominal = SILRunner{config}.run(nominal_scenarios);
    const std::expected<SilRunOutput, SilError> faulted = SILRunner{config}.run(fault_scenarios);

    if (!nominal.has_value() || !faulted.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }

    check_fault_window(runner, faulted.value(), config);
    check_commands_diverge(runner, nominal.value(), faulted.value(), kFaultStart, kFaultEnd);

    const SilEvent* cleared = find_first(faulted.value().events, SilEventType::FaultCleared);

    runner.check(cleared != nullptr, "FAULT_CLEARED event present");
    if (cleared == nullptr) {
        return;
    }
    runner.check(std::abs(cleared->timestamp - kFaultEnd) <= config.dt, "FAULT_CLEARED at window close");
    runner.check(cleared->detail == sim::sil::failure_mode_name(FailureMode::INVALID_SENSOR_DATA),
                 "FAULT_CLEARED carries the failure mode");
}

}
