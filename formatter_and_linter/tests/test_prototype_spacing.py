#!/usr/bin/env python3
"""
Prototype Spacing Tests

Unit tests for the prototype spacing / character cleaning pass, including
the blank-line collapsing between consecutive function prototypes.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.prototype_spacing import clean_code


RAW_INPUT = (
    "namespace sim::test::sil {\n"
    "\n"
    "using sim::safety::FaultDomain;\n"
    "using sim::sil::FaultScenario;\n"
    "\n"
    "SimulationResult run_case(const std::vector<FaultScenario>& scenarios);\n"
    "\n"
    "\n"
    "void nominal_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);\n"
    "\n"
    "void fc1_failure_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);\n"
    " \n"
    "\t\n"
    "void sensor_fault_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)\n"
    "{\n"
    "    std::cout << \"\\n=== SIL-004 ===\" << std::endl;\n"
    "\n"
    "\n"
    "    FaultDomain domain;\n"
    "\n"
    "    domain.activate();\n"
    "}\n"
    "\n"
    "}"
)

EXPECTED_OUTPUT = (
    "namespace sim::test::sil {\n"
    "\n"
    "using sim::safety::FaultDomain;\n"
    "using sim::sil::FaultScenario;\n"
    "\n"
    "SimulationResult run_case(const std::vector<FaultScenario>& scenarios);\n"
    "void nominal_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);\n"
    "void fc1_failure_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records);\n"
    "\n"
    "void sensor_fault_scenario(TestHarness& runner, std::vector<ScenarioRecord>& records)\n"
    "{\n"
    "    std::cout << \"\\n=== SIL-004 ===\" << std::endl;\n"
    "\n"
    "\n"
    "    FaultDomain domain;\n"
    "\n"
    "    domain.activate();\n"
    "}\n"
    "\n"
    "}"
)


class TestPrototypeSpacing(unittest.TestCase):

    def test_user_example_is_cleaned(self):
        self.assertEqual(clean_code(RAW_INPUT), EXPECTED_OUTPUT)

    def test_non_breaking_spaces_are_replaced(self):
        code = "void f(int value);\xa0\xa0\n"
        self.assertEqual(clean_code(code), "void f(int value);\n")

    def test_trailing_whitespace_is_removed(self):
        code = "int compute(int value);   \n\t\nint other(int value);\n"
        self.assertEqual(clean_code(code), "int compute(int value);\nint other(int value);\n")

    def test_blank_line_kept_before_multiline_definition(self):
        code = (
            "void first(int value);\n"
            "\n"
            "void second(\n"
            "    int value)\n"
            "{\n"
            "    do_work();\n"
            "}\n"
        )
        expected = (
            "void first(int value);\n"
            "\n"
            "void second(\n"
            "    int value)\n"
            "{\n"
            "    do_work();\n"
            "}\n"
        )
        self.assertEqual(clean_code(code), expected)

    def test_blank_lines_collapsed_between_multiline_prototypes(self):
        code = (
            "Result run(\n"
            "    const Config& config);\n"
            "\n"
            "Result check(\n"
            "    const Config& config);\n"
        )
        expected = (
            "Result run(\n"
            "    const Config& config);\n"
            "Result check(\n"
            "    const Config& config);\n"
        )
        self.assertEqual(clean_code(code), expected)

    def test_blank_kept_between_prototype_and_using_block(self):
        code = (
            "void first(int value);\n"
            "\n"
            "using sim::sil::FaultScenario;\n"
            "\n"
            "void second(int value);\n"
        )
        self.assertEqual(clean_code(code), code)

    def test_blank_lines_inside_function_body_preserved(self):
        code = (
            "void run(int value)\n"
            "{\n"
            "    int a = 1;\n"
            "\n"
            "    int b = 2;\n"
            "\n"
            "\n"
            "    int c = 3;\n"
            "}\n"
        )
        self.assertEqual(clean_code(code), code)

    def test_comments_prototypes_and_block_comment(self):
        code = (
            "/*\n"
            "Filename: Src/Test.cpp\n"
            "*/\n"
            "\n"
            "void first(int value); // trailing comment\n"
            "\n"
            "void second(int value);\n"
        )
        expected = (
            "/*\n"
            "Filename: Src/Test.cpp\n"
            "*/\n"
            "\n"
            "void first(int value); // trailing comment\n"
            "void second(int value);\n"
        )
        self.assertEqual(clean_code(code), expected)

    def test_idempotent(self):
        self.assertEqual(clean_code(EXPECTED_OUTPUT), EXPECTED_OUTPUT)


if __name__ == "__main__":
    unittest.main()
