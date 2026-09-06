/*
Filename: Tests/Sil/Observability/Faults/SilObservability-FaultMetadata.cpp
Description: Fault injection metadata tests : target identity, temporality profile and expected behavior.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservability;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultProfile;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SensorCorruptionMode;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEvent;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;

/*
OBS-011: a permanent actuator degradation logs the precise target and disturbed
signal, the explicit PERMANENT profile without bounded duration and the nominal
compensation response expected from the safety chain.
*/
void fault_metadata_test(TestHarness& runner)
{
    runner.set_context("OBS-011");
    const SilConfig config{.duration_s = 30.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 5.0,
                                 .fault_type = FaultType::ActuatorDegradation,
                                 .parameters = {.efficiency = 0.6}};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }
    const SilEvent* injected = find_first(outcome.value().events, SilEventType::FaultInjected);
    if (injected == nullptr) {
        runner.check(false, "FAULT_INJECTED event present");
        return;
    }

    runner.check(injected->target == "actuator_0" && injected->target_signal == "wing_rpm",
                 "actuator target identified with its signal");
    runner.check(injected->physical_role == "main_rotor" && injected->physical_category == "ROTOR_MOTOR",
                 "actuator physical role and hardware category");
    runner.check(injected->physical_function == "Propulsion / Roll-Pitch-Yaw Control",
                 "actuator aerodynamic function documented");
    runner.check(injected->has_profile && injected->profile == FaultProfile::Permanent, "explicit PERMANENT profile");
    runner.check(!injected->has_duration, "permanent fault without bounded duration");
    runner.check(injected->value_kind == sim::sil::FaultValueKind::Efficiency
                     && std::abs(injected->value - 0.6) <= 1.0e-9,
                 "efficiency parameter carried and typed");
    runner.check(injected->expected_behavior.find("ACTUATOR_MISMATCH") != std::string_view::npos
                     && injected->expected_behavior.find("COMPENSATED") != std::string_view::npos,
                 "expected behavior documented");
}

/*
OBS-012: a temporary sensor corruption logs the baro target, the corruption
subtype, the TEMPORARY profile with its explicit duration and the fault-cleared
marker repeating the target identity.
*/
void sensor_fault_metadata_test(TestHarness& runner)
{
    runner.set_context("OBS-012");
    const SilConfig config{.duration_s = 30.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 10.0,
                                 .duration = 5.0,
                                 .fault_type = FaultType::SensorFault,
                                 .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange,
                                                .corrupted_altitude_m = 99999.0}};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "SIL runner failed");
        return;
    }
    const SilEvent* injected = find_first(outcome.value().events, SilEventType::FaultInjected);
    const SilEvent* cleared = find_first(outcome.value().events, SilEventType::FaultCleared);
    if (injected == nullptr || cleared == nullptr) {
        runner.check(false, "FAULT_INJECTED and FAULT_CLEARED events present");
        return;
    }

    runner.check(injected->target == "baro_primary", "baro sensor target identified");
    runner.check(injected->physical_role.empty() && injected->physical_category.empty(),
                 "non-actuator target without physical role");
    runner.check(injected->subtype == "ALTITUDE_OUT_OF_RANGE", "corruption subtype carried");
    runner.check(injected->has_profile && injected->profile == FaultProfile::Temporary, "explicit TEMPORARY profile");
    runner.check(injected->has_duration && std::abs(injected->duration_s - 5.0) <= 1.0e-9,
                 "explicit duration of the temporary fault");
    runner.check(!injected->expected_behavior.empty(), "expected behavior documented");
    runner.check(cleared->target == "baro_primary" && cleared->has_profile
                     && cleared->profile == FaultProfile::Temporary,
                 "FAULT_CLEARED carries the target and profile");
}

}
