/*
Filename: Tests/Hil/HilTests-Core.cpp
Description: Orchestration of the deterministic HIL test suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module HilTests;

import std;

import TestHarness;

namespace sim::test::hil {

void run_all_hil_tests(sim::test::TestHarness& runner)
{
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
