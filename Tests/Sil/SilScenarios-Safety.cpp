/*
Filename: Tests/Sil/SilScenarios-Safety.cpp
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
using sim::sil::ScenarioRecord;
using sim::sil::SensorCorruptionMode;
using sim::sil::SilError;
using sim::sil::SimulationResult;

std::expected<SimulationResult, SilError> run_case(std::span<const FaultScenario> scenarios);
void nominal_scenario(TestHarness& runner, ScenarioRecord& record);
void fc1_failure_scenario(TestHarness& runner, ScenarioRecord& record);
void communication_loss_scenario(TestHarness& runner, ScenarioRecord& record);

void sensor_fault_scenario(TestHarness& runner, ScenarioRecord& record)
{
    std::cout << "\n=== SIL-004 : capteur d'altitude corrompu a t = 20.0 s ===" << std::endl;
    const FaultScenario scenario{ .start_time = 20.0, .duration = 10.0, .fault_type = FaultType::SensorFault,
        .parameters = {.corruption = SensorCorruptionMode::AltitudeOutOfRange, .corrupted_altitude_m = 99999.0}};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SimulationResult, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL-004 : moteur SIL en echec");
        record = {.name = "SIL-004", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    SimulationResult r = outcome.value();
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Sensor,
                 "SIL-004 : capteur d'altitude invalide par la validation");
    runner.check(r.detection_latency >= 0.0 && r.detection_latency <= 0.50,
                 "SIL-004 : invalidation immediate (<= 500 ms)");
    runner.check(r.degraded_reached, "SIL-004 : HealthMonitor en etat DEGRADED");
    runner.check(r.compensated_reached, "SIL-004 : mode COMPENSATED engage");
    runner.check(!r.mission_aborted, "SIL-004 : mission non annulee");
    record = {.name = "SIL-004", .scenario = scenario, .result = r};
}

void actuator_degradation_scenario(TestHarness& runner, ScenarioRecord& record)
{
    std::cout << "\n=== SIL-005 : rendement actionneur 0.6 a t = 15.0 s ===" << std::endl;
    const FaultScenario scenario{.start_time = 15.0, .duration = 0.0, .fault_type = FaultType::ActuatorDegradation,
                                 .parameters = {.efficiency = 0.6}};
    const std::array<FaultScenario, 1> scenarios{scenario};

    const std::expected<SimulationResult, SilError> outcome = run_case(scenarios);
    if (!outcome.has_value()) {
        runner.check(false, "SIL-005 : moteur SIL en echec");
        record = {.name = "SIL-005", .scenario = scenario, .result = SimulationResult{}};
        return;
    }

    SimulationResult r = outcome.value();
    r.passed = r.compute_verdict(true);
    runner.check(r.fault_detected && r.first_fault_domain == FaultDomain::Actuator,
                 "SIL-005 : desaccord commande/reponse detecte");
    runner.check(r.degraded_reached, "SIL-005 : HealthMonitor en etat DEGRADED");
    runner.check(r.compensated_reached, "SIL-005 : consigne de compensation engagee");
    runner.check(!r.mission_aborted, "SIL-005 : mission non annulee");
    record = {.name = "SIL-005", .scenario = scenario, .result = r};
}

/*
Orchestration: runs the five scenarios into the fixed record array, generates the
docs/validation/sil.md, .json, .csv artifacts through the typed-error writer and
echoes the Markdown report on the console.
*/
void run_all_sil_scenarios(TestHarness& runner)
{
    std::array<ScenarioRecord, 5> records{};

    nominal_scenario(runner, records[0]);
    fc1_failure_scenario(runner, records[1]);
    communication_loss_scenario(runner, records[2]);
    sensor_fault_scenario(runner, records[3]);
    actuator_degradation_scenario(runner, records[4]);

    std::cout << "\n=== Generation du rapport SIL (docs/validation) ===" << std::endl;
    const std::expected<void, sim::sil::ReportError> outcome = sim::sil::write_sil_report(records);
    if (!outcome.has_value()) {
        runner.check(false, "generation des artefacts SIL impossible");
        return;
    }
    std::cout << "Artefacts generes : sil.md, sil.json, sil.csv" << std::endl;
    sim::sil::write_markdown_report(std::cout, records);
}

}
