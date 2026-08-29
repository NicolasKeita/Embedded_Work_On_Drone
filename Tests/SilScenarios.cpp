/*
Filename: Tests/SilScenarios.cpp
Description: SIL scenarios 001 to 003 : nominal, FC1 failure and communication loss.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import Aircraft;
import FlightController;
import TestHarness;
import Telemetry;
import HealthMonitor;
import SafetyManager;
import SilTypes;
import SilRunner;
import SilReporting;

namespace sim::test::sil {

using sim::safety::FaultDomain;
using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::ScenarioRecord;
using sim::sil::SimulationResult;
using sim::sil::SILRunner;

/*
Cas de test : mission de montee vers 10 m avec la configuration SIL par defaut
(dt = 10 ms, duree 90 s), faultes optionnelles appliquees par le moteur.
*/
SimulationResult run_case(const std::vector<FaultScenario>& scenarios)
{
    return SILRunner{}.run(scenarios);
}

void nominal_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)
{
    std::cout << "\n=== SIL-001 : vol nominal sans faulte ===" << std::endl;
    const FaultScenario scenario{};
    SimulationResult r = run_case({scenario});
    r.passed = r.compute_verdict(false);
    runner.check(r.mission_success, "SIL-001 : mission COMPLETE sans faulte");
    runner.check(r.final_safety_mode == SafetyMode::NORMAL,
                 "SIL-001 : mode de surete NORMAL");
    runner.check(!r.fault_detected, "SIL-001 : aucune faulte detectee");
    runner.check(r.max_altitude_error_m <= 10.5,
                 "SIL-001 : erreur d'altitude maitrisee (<= 10.5 m)");
    records.push_back({"SIL-001", scenario, r});
}

void fc1_failure_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)
{
    std::cout << "\n=== SIL-002 : defaillance FC1 a t = 30.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 30.0, .duration = 0.0,
                                 .fault_type = FaultType::FC1Failure};
    SimulationResult r = run_case({scenario});
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected, "SIL-002 : defaillance FC1 detectee");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30,
                 "SIL-002 : timeout heartbeat en <= 300 ms");
    runner.check(r.final_safety_mode == SafetyMode::SAFE_MODE,
                 "SIL-002 : passage en SAFE_MODE");
    runner.check(r.response_latency >= 0.0 && r.response_latency <= 0.20,
                 "SIL-002 : latence de reponse <= 200 ms");
    runner.check(r.mission_aborted, "SIL-002 : mission annulee par le SafetyManager");
    records.push_back({"SIL-002", scenario, r});
}

void communication_loss_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)
{
    std::cout << "\n=== SIL-003 : perte de communication a t = 30.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 30.0, .duration = 0.0,
                                 .fault_type = FaultType::CommunicationLoss};
    SimulationResult r = run_case({scenario});
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected
                     && r.first_fault_domain == FaultDomain::Communication,
                 "SIL-003 : alerte COMMUNICATION_LOST levee");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.30,
                 "SIL-003 : alerte en <= 300 ms");
    runner.check(r.safe_mode_reached, "SIL-003 : reaction de surete engagee");
    runner.check(r.mission_aborted, "SIL-003 : mission annulee (regle etape 10)");
    records.push_back({"SIL-003", scenario, r});
}

}
