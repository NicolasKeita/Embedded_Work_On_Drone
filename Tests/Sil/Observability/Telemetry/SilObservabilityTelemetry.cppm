/*
Filename: Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry.cppm
Description: Interface of the telemetry-path observability suite (sensor chain tests).
Exports:
    telemetry_sampling_test(),
    telemetry_sensor_fault_path_test(),
    telemetry_sensor_hold_and_clear_test(),
    telemetry_rate_neutrality_test(),
    run_telemetry_scenarios()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilObservabilityTelemetry;

import TestHarness;

export namespace sim::test::sil {

// Telemetry-path suite (sensor chain observability).
void telemetry_sampling_test(TestHarness& runner);
void telemetry_sensor_fault_path_test(TestHarness& runner);
void telemetry_sensor_hold_and_clear_test(TestHarness& runner);
void telemetry_rate_neutrality_test(TestHarness& runner);

// Runs the whole telemetry-path observability suite.
void run_telemetry_scenarios(TestHarness& runner);

}