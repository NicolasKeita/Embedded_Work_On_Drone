/*
Filename: Tests/Sil/Observability/SilScenarios-Observability-Fields.cpp
Description: Structured SimulationResult field population test.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import SilEvents;
import SilRunner;
import SilTypes;
import TestHarness;

namespace sim::test::sil {

using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SilConfig;
using sim::sil::SilError;
using sim::sil::SilEventType;
using sim::sil::SilLogLevel;
using sim::sil::SilRunOutput;
using sim::sil::SimulationResult;

/*
OBS-008: the structured SimulationResult carries the full fault chain and the
aircraft/metric aggregates populated at every run.
*/
void structured_fields_test(TestHarness& runner)
{
    const SilConfig config{.duration_s = 60.0, .trace_level = SilLogLevel::Trace};
    const FaultScenario scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);

    if (!outcome.has_value()) {
        runner.check(false, "OBS-008 : moteur SIL en echec");
        return;
    }

    const SimulationResult& result = outcome.value().result;

    runner.check(result.fault_injected_time >= 30.0 && result.fault_detected,
                 "OBS-008 : chaine d'injection detectee");
    runner.check(result.detection_time >= result.fault_injected_time,
                 "OBS-008 : detection apres injection");
    runner.check(std::abs(result.detection_latency
                          - (result.detection_time - result.fault_injected_time)) < 1.0e-9,
                 "OBS-008 : latence de detection coherencee");
    runner.check(result.response_latency >= 0.0 && result.safety_response_time >= result.detection_time,
                 "OBS-008 : latence de reponse coherencee");
    runner.check(std::isfinite(result.final_altitude_m) && result.max_altitude_error_m >= 0.0,
                 "OBS-008 : champs aeronefs renseignes");
    runner.check(result.max_position_error_m >= 0.0 && result.mean_altitude_error_m >= 0.0,
                 "OBS-008 : agregats d'erreur renseignes");
    runner.check(outcome.value().telemetry.size() > 0 && outcome.value().events.size() > 10,
                 "OBS-008 : telemetrie et trace structurees presentes");
}

}