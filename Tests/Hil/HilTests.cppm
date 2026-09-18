/*
Filename: Tests/Hil/HilTests.cppm
Description: Interface of the deterministic HIL test suite (runner, protocol, data-integrity
and fault tests).
Export summary: run_all_hil_tests()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilTests;

import std;

import HilConfig;
import TestHarness;

export namespace sim::test::hil {

void run_runner_tests(sim::test::TestHarness& runner);
void run_protocol_tests(sim::test::TestHarness& runner);
void run_data_tests(sim::test::TestHarness& runner);
void run_fault_tests(sim::test::TestHarness& runner);
void run_all_hil_tests(sim::test::TestHarness& runner);

}

namespace sim::test::hil {

/* Transport-layer protocol tests (module-internal, driven by run_protocol_tests). */
void run_transport_tests(sim::test::TestHarness& runner);

/* Scenario configuration helper (module-internal, shared by the HIL test units). */
sim::hil::HilConfig test_config(std::string_view id, std::float64_t duration_s);

/* Wind disturbance tests (module-internal, driven by run_runner_tests). */
void test_wind(sim::test::TestHarness& runner);

/* FC2 supervision rearm tests (module-internal, driven by run_fault_tests). */
void test_supervision_rearm(sim::test::TestHarness& runner);

}
