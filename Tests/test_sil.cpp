/*
Filename: Tests/test_sil.cpp
Description: Entry point running the deterministic SIL fault injection test suite.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import TestHarness;
import SilScenarios;

int main()
{
    sim::test::TestHarness runner;
    sim::test::sil::run_all_sil_scenarios(runner);

    if (runner.passed()) {
        std::cout << "\nTous les tests SIL sont passes." << std::endl;
        return 0;
    }
    std::cout << "\n" << runner.failure_count() << " echec(s) dans la suite SIL." << std::endl;
    return 1;
}
