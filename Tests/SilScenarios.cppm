/*
Filename: Tests/SilScenarios.cppm
Description: Interface of the deterministic SIL test suite (SIL-001 to SIL-005).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilScenarios;

import std;

import TestHarness;

export namespace sim::test::sil {

void run_all_sil_scenarios(TestHarness& runner);

}
