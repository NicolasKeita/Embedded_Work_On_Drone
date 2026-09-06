/*
Filename: Tests/test_hil.cpp
Description: Entry point running the deterministic HIL test suite (runner, protocol,
data-integrity and fault tests).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

import std;

import HilTests;
import TestHarness;

int main()
{
    sim::test::TestHarness runner;

    sim::test::hil::run_all_hil_tests(runner);

    if (runner.passed()) {
        std::cout << "\nAll HIL tests passed." << std::endl;
        return 0;
    }
    std::cout << "\n" << runner.failure_count() << " HIL test failure(s)." << std::endl;
    return 1;
}
