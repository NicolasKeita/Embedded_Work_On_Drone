/*
Filename: Tests/Hil/HilTests-Core.cpp
Description: Orchestration of the deterministic HIL test suite, including the protocol test
dispatch over the HilProtoTests module.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import HilHardwareConfigTests;
import HilProtoTests;
import RuntimeConfigTests;
import TestHarness;

namespace sim::test::hil {

void run_protocol_tests(sim::test::TestHarness& runner)
{
    runner.set_context("PROTO");
    run_transport_tests(runner);
    run_proto_inter_fc_tests(runner);
    run_proto_setpoint_tests(runner);
    run_proto_dynamic_setpoint_tests(runner);
    run_proto_version_tests(runner);
}

void run_all_hil_tests(sim::test::TestHarness& runner)
{
    std::cout << "\n=== Host runtime configuration tests ===" << std::endl;
    sim::test::config::run_runtime_config_tests(runner);

    std::cout << "\n=== HIL hardware configuration tests ===" << std::endl;
    sim::test::hil::run_hardware_config_tests(runner);

    std::cout << "\n=== HIL runner tests ===" << std::endl;
    run_runner_tests(runner);

    std::cout << "\n=== HIL protocol tests ===" << std::endl;
    run_protocol_tests(runner);

    std::cout << "\n=== HIL data-integrity tests ===" << std::endl;
    run_data_tests(runner);

    std::cout << "\n=== HIL fault tests ===" << std::endl;
    run_fault_tests(runner);
}

}
