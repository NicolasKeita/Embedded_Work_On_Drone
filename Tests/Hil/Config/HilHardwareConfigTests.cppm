/*
Filename: Tests/Hil/Config/HilHardwareConfigTests.cppm
Description: Interface of the HIL hardware identity configuration parser tests.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilHardwareConfigTests;

import TestHarness;

export namespace sim::test::hil {

/* Verifies hardware identity parsing, normalization and invalid-input rejection. */
void run_hardware_config_tests(sim::test::TestHarness& runner);

}
