#!/usr/bin/env python3
"""
Multiple Variable Declaration Tests

Unit tests for the multiple-variable-declaration style check.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.multiple_var_decl_checks import check_multiple_var_declarations, MULTIPLE_VAR_DECL_MESSAGE


def check_code(code: str):
    return check_multiple_var_declarations('\n'.join(code))


class TestMultipleVarDeclarations(unittest.TestCase):

    def test_grouped_declarations_are_reported(self):
        code = [
            'AircraftState fc1_view{}, sampled_truth{};',
            'bool fc1_was_alive = true, fault_active = false, fault_recorded = false;',
            'std::float64_t commanded_rpm = 0.0, last_effective_rpm = 0.0, safe_rpm = 0.0;',
        ]
        self.assertEqual(check_code(code), [1, 2, 3])

    def test_separate_lines_are_valid(self):
        code = [
            'AircraftState fc1_view{};',
            'AircraftState sampled_truth{};',
            'std::float64_t commanded_rpm = 0.0;',
        ]
        self.assertEqual(check_code(code), [])

    def test_template_comma_is_not_a_violation(self):
        code = [
            'std::array<bool, 4> previous_flags{};',
        ]
        self.assertEqual(check_code(code), [])

    def test_aggregate_brace_commas_are_not_a_violation(self):
        code = [
            'sim::control::TargetState target{.z = 10.0, .x = 1.0};',
        ]
        self.assertEqual(check_code(code), [])

    def test_function_call_commas_are_not_a_violation(self):
        code = [
            'simulate(input, output, config);',
            'auto result = compute(a, b);',
        ]
        self.assertEqual(check_code(code), [])

    def test_comma_in_initializer_list_is_not_a_violation(self):
        code = [
            'std::array<int, 3> values = {1, 2, 3};',
        ]
        self.assertEqual(check_code(code), [])

    def test_comma_inside_comments_and_strings_is_ignored(self):
        code = [
            '// int a, b;',
            'std::string message = "a, b";',
            '/* bool x = true, y = false; */',
        ]
        self.assertEqual(check_code(code), [])

    def test_nested_template_comma_is_not_a_violation(self):
        code = [
            'std::map<std::uint32_t, std::string> lookup{};',
        ]
        self.assertEqual(check_code(code), [])

    def test_for_statement_comma_is_not_a_violation(self):
        code = [
            'for (std::size_t i = 0, j = 1; i < n; ++i) {',
            '    step(i, j);',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_preprocessor_lines_are_ignored(self):
        code = [
            '#define PAIR(a, b) a, b',
        ]
        self.assertEqual(check_code(code), [])

    def test_message_text(self):
        self.assertEqual(MULTIPLE_VAR_DECL_MESSAGE, "[MULTIPLE_VAR_DECL] Declare only one variable per line.")


if __name__ == "__main__":
    unittest.main()
