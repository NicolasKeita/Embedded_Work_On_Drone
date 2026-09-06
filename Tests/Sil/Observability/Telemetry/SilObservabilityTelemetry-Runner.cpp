/*
Filename: Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Runner.cpp
Description: Orchestration of the telemetry-path observability suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilObservabilityTelemetry;

import std;

import TestHarness;

namespace sim::test::sil {

/*
Runs the whole telemetry-path observability suite.
*/
void run_telemetry_scenarios(TestHarness& runner)
{
    std::cout << "\n=== SIL telemetry observability suite ===" << std::endl;

    telemetry_sampling_test(runner);
    telemetry_sensor_fault_path_test(runner);
    telemetry_sensor_hold_and_clear_test(runner);
    telemetry_rate_neutrality_test(runner);
}

}
