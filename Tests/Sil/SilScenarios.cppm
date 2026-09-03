/*
Filename: Tests/Sil/SilScenarios.cppm
Description: Interface of the deterministic SIL test suite (SIL-001 to SIL-005); observability tests live in the SilObservability module.
Exports:
    run_all_sil_scenarios()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilScenarios;

import TestHarness;

export namespace sim::test::sil {

void run_all_sil_scenarios(TestHarness& runner);

}
