/*
Filename: Tests/Config/RuntimeConfigTests.cppm
Description: Host configuration parser, validation, and simulation parameter regression tests.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module RuntimeConfigTests;

import std;

import TestHarness;

export namespace sim::test::config
{
    /* Exercises host configuration without files, devices, or real-time execution. */
    void run_runtime_config_tests(sim::test::TestHarness& runner);
}
