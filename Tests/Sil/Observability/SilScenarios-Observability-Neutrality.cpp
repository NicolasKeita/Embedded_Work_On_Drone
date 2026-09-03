/*
Filename: Tests/Sil/Observability/SilScenarios-Observability-Neutrality.cpp
Description: Logging neutrality test: verbosity never alters the simulation result.

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
OBS-009: lowering the trace verbosity never changes the simulation result, only
the richness of the recorded event trace (observational neutrality).
*/
void logging_neutrality_test(TestHarness& runner)
{
    const FaultScenario scenario{.start_time = 30.0, .fault_type = FaultType::FC1Failure};
    const SilConfig info_cfg{.duration_s = 35.0};
    const SilConfig trace_cfg{.duration_s = 35.0, .trace_level = SilLogLevel::Trace};
    const std::expected<SilRunOutput, SilError> info = run_traced(info_cfg, scenario);
    const std::expected<SilRunOutput, SilError> trace = run_traced(trace_cfg, scenario);

    if (!info.has_value() || !trace.has_value()) {
        runner.check(false, "OBS-009 : moteur SIL en echec");
        return;
    }

    const SimulationResult& i = info.value().result;
    const SimulationResult& t = trace.value().result;

    runner.check(i.mission_success == t.mission_success, "OBS-009 : succes mission independant");
    runner.check(i.final_state == t.final_state, "OBS-009 : etat terminal independant");
    runner.check(i.detection_latency == t.detection_latency, "OBS-009 : latence de detection identique");
    runner.check(i.comms.sent == t.comms.sent && i.comms.dropped == t.comms.dropped,
                 "OBS-009 : statistiques comm identiques");
    runner.check(i.max_altitude_error_m == t.max_altitude_error_m, "OBS-009 : metriques aircraft identiques");
    runner.check(count_events(info.value().events, SilEventType::HeartbeatSent)
                 < count_events(trace.value().events, SilEventType::HeartbeatSent),
                 "OBS-009 : trace verbeux plus riche sans modifier le resultat");
}

}
