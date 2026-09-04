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
    const SilConfig config{.duration_s = 30.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 5.0,
                                 .fault_type = FaultType::ActuatorDegradation,
                                 .parameters = {.efficiency = 0.6}};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-011 : moteur SIL en echec");
        return;
    }
    const SilEvent* injected = find_first(outcome.value().events, SilEventType::FaultInjected);
    if (injected == nullptr) {
        runner.check(false, "OBS-011 : evenement FAULT_INJECTED present");
        return;
    }

    runner.check(injected->target == "actuator_0" && injected->target_signal == "wing_rpm",
                 "OBS-011 : cible actionneur identifiee avec son signal");
    runner.check(injected->has_profile && injected->profile == FaultProfile::Permanent,
                 "OBS-011 : profil PERMANENT explicite");
    runner.check(!injected->has_duration, "OBS-011 : faute permanente sans duree bornee");
    runner.check(injected->value_kind == sim::sil::FaultValueKind::Efficiency
                     && std::abs(injected->value - 0.6) <= 1.0e-9,
                 "OBS-011 : parametre efficiency porte et type");
    runner.check(injected->expected_behavior.find("ACTUATOR_MISMATCH") != std::string_view::npos
                     && injected->expected_behavior.find("COMPENSATED") != std::string_view::npos,
                 "OBS-011 : comportement attendu documente");
}

/*
OBS-012: a temporary sensor corruption logs the baro target, the corruption
subtype, the TEMPORARY profile with its explicit duration and the fault-cleared
marker repeating the target identity.
*/
void sensor_fault_metadata_test(TestHarness& runner)
{
    const SilConfig config{.duration_s = 30.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 10.0,
                                 .duration = 5.0,
                                 .fault_type = FaultType::SensorFault,
                                 .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange,
                                                .corrupted_altitude_m = 99999.0}};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-012 : moteur SIL en echec");
        return;
    }
    const SilEvent* injected = find_first(outcome.value().events, SilEventType::FaultInjected);
    const SilEvent* cleared = find_first(outcome.value().events, SilEventType::FaultCleared);
    if (injected == nullptr || cleared == nullptr) {
        runner.check(false, "OBS-012 : evenements FAULT_INJECTED et FAULT_CLEARED presents");
        return;
    }

    runner.check(injected->target == "baro_primary", "OBS-012 : cible capteur baro identifiee");
    runner.check(injected->subtype == "ALTITUDE_OUT_OF_RANGE", "OBS-012 : sous-type de corruption porte");
    runner.check(injected->has_profile && injected->profile == FaultProfile::Temporary,
                 "OBS-012 : profil TEMPORARY explicite");
    runner.check(injected->has_duration && std::abs(injected->duration_s - 5.0) <= 1.0e-9,
                 "OBS-012 : duree explicite de la faute temporaire");
    runner.check(!injected->expected_behavior.empty(), "OBS-012 : comportement attendu documente");
    runner.check(cleared->target == "baro_primary" && cleared->has_profile
                     && cleared->profile == FaultProfile::Temporary,
                 "OBS-012 : FAULT_CLEARED porte la cible et le profil");
}

}