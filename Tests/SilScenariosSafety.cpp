/*
Filename: Tests/SilScenariosSafety.cpp
Description: SIL scenarios 004 and 005, report generation and suite orchestration.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilScenarios;

import std;

import Aircraft;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilReporting;
import SilRunner;
import SilTypes;
import Telemetry;
import TestHarness;

namespace sim::test::sil {

using sim::safety::FaultDomain;
using sim::safety::SafetyMode;
using sim::sil::FaultScenario;
using sim::sil::FaultType;
using sim::sil::SensorCorruptionMode;
using sim::sil::ScenarioRecord;
using sim::sil::SimulationResult;

SimulationResult run_case(const std::vector<FaultScenario>& scenarios);

void nominal_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);

void fc1_failure_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);

void communication_loss_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);

void sensor_fault_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)
{
    std::cout << "\n=== SIL-004 : capteur d'altitude corrompu a t = 20.0 s ===" << std::endl;
    const FaultScenario scenario{
        .start_time = 20.0, .duration = 10.0, .fault_type = FaultType::SensorFault,
        .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange,
                       .corrupted_altitude_m = 99999.0}};
    SimulationResult r = run_case({scenario});
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Sensor,
                 "SIL-004 : capteur d'altitude invalide par la validation");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.50,
                 "SIL-004 : invalidation immediate (<= 500 ms)");
    runner.check(r.degraded_reached, "SIL-004 : HealthMonitor en etat DEGRADED");
    runner.check(r.compensated_reached, "SIL-004 : mode COMPENSATED engage");
    runner.check(!r.mission_aborted, "SIL-004 : mission non annulee");
    records.push_back({"SIL-004", scenario, r});
}

void actuator_degradation_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)
{
    std::cout << "\n=== SIL-005 : rendement actionneur 0.6 a t = 15.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 15.0, .duration = 0.0,
                                 .fault_type = FaultType::ActuatorDegradation,
                                 .parameters = {.efficiency = 0.6}};
    SimulationResult r = run_case({scenario});
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Actuator,
                 "SIL-005 : desaccord commande/reponse detecte");
    runner.check(r.degraded_reached, "SIL-005 : HealthMonitor en etat DEGRADED");
    runner.check(r.compensated_reached, "SIL-005 : consigne de compensation engagee");
    runner.check(!r.mission_aborted, "SIL-005 : mission non annulee");
    records.push_back({"SIL-005", scenario, r});
}

/*
Orchestration : execution des cinq scenarios, consolidation des verdicts dans
les records puis generation des artefacts docs/validation/sil.md, .json, .csv.
*/
void run_all_sil_scenarios(TestHarness& runner)
{
    std::vector<ScenarioRecord> records;
    nominal_scenario(runner, records);
    fc1_failure_scenario(runner, records);
    communication_loss_scenario(runner, records);
    sensor_fault_scenario(runner, records);
    actuator_degradation_scenario(runner, records);

    std::cout << "\n=== Generation du rapport SIL (docs/validation) ===" << std::endl;
    const std::string markdown = sim::sil::write_sil_report(records);
    std::cout << markdown << std::endl;
}

}
