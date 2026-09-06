/*
Filename: Tests/Hil/HilTests.cppm
Description: Interface of the deterministic HIL test suite (runner, protocol, data-integrity
and fault tests).
Export summary: run_all_hil_tests()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilTests;

import TestHarness;

export namespace sim::test::hil {

void run_runner_tests(sim::test::TestHarness& runner);
void run_protocol_tests(sim::test::TestHarness& runner);
void run_data_tests(sim::test::TestHarness& runner);
void run_fault_tests(sim::test::TestHarness& runner);
void run_all_hil_tests(sim::test::TestHarness& runner);

}
