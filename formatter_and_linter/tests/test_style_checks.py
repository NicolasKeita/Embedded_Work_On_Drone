#!/usr/bin/env python3
"""
Style Checks Tests

Unit tests for the function length check of the C++ linter, including
multi-line function signatures.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.style_checks import MAX_FUNCTION_LENGTH, check_function_length


def check_lines(lines):
    return check_function_length('\n'.join(lines))


def build_function(signature_lines, body_line_count):
    body = ['    int sample_value_' + str(i) + ' = ' + str(i) + ';' for i in range(body_line_count)]
    return signature_lines + ['{'] + body + ['}']


class TestCheckFunctionLength(unittest.TestCase):

    def test_long_function_with_single_line_signature_is_reported(self):
        code = build_function(['int compute(int value)'], 40)
        long_functions = check_lines(code)
        self.assertEqual(len(long_functions), 1)
        self.assertEqual(long_functions[0][0], 'compute')
        self.assertEqual(long_functions[0][1], 1)

    def test_long_function_with_multiline_signature_is_reported(self):
        signature_lines = [
            'MissionRunTrace run_mission(FlightController&        ctrl,',
            '                            Aircraft&                craft,',
            '                            const MissionRunRequest& run)',
        ]
        code = build_function(signature_lines, 40)
        long_functions = check_lines(code)
        self.assertEqual(len(long_functions), 1)
        self.assertEqual(long_functions[0][0], 'run_mission')
        self.assertEqual(long_functions[0][1], 1)
        self.assertEqual(long_functions[0][2], 45)

    def test_short_function_with_multiline_signature_is_not_reported(self):
        signature_lines = [
            'MissionRunTrace run_mission(FlightController&        ctrl,',
            '                            Aircraft&                craft,',
            '                            const MissionRunRequest& run)',
        ]
        code = build_function(signature_lines, 5)
        self.assertEqual(check_lines(code), [])

    def test_multiline_signature_with_same_line_brace_is_reported(self):
        code = [
            'void run_mission(FlightController& ctrl,',
            '                 const MissionRunRequest& run) {',
        ]
        code += ['    int sample_value_' + str(i) + ' = ' + str(i) + ';' for i in range(40)]
        code += ['}']
        long_functions = check_lines(code)
        self.assertEqual(len(long_functions), 1)
        self.assertEqual(long_functions[0][0], 'run_mission')

    def test_multiline_control_structure_is_not_reported(self):
        code = [
            'int compute(int value)',
            '{',
            '    while (value > 0',
            '            && value < 100)',
            '    {',
            '        --value;',
            '    }',
            '    return value;',
            '}',
        ]
        self.assertEqual(check_lines(code), [])

    def test_long_class_body_is_not_reported(self):
        code = ['class Data', '{']
        code += ['    int field_' + str(i) + ' = ' + str(i) + ';' for i in range(40)]
        code += ['};']
        self.assertEqual(check_lines(code), [])

    def test_long_namespace_body_is_not_reported(self):
        code = ['namespace sim::test {']
        code += ['int value_' + str(i) + ' = ' + str(i) + ';' for i in range(40)]
        code += ['}']
        self.assertEqual(check_lines(code), [])

    def test_braces_in_strings_are_ignored(self):
        body = ['    std::cout << "row {' + str(i) + '}";' for i in range(3)]
        code = ['void print_table()', '{'] + body + ['}']
        self.assertEqual(check_lines(code), [])

    def test_lambda_body_is_not_reported(self):
        code = [
            'int compute(int value)',
            '{',
            '    auto scale = [&](int input) {',
            '        return input * 2;',
            '    };',
            '    return scale(value);',
            '}',
        ]
        self.assertEqual(check_lines(code), [])

    def test_default_max_function_length_is_enforced(self):
        code = build_function(['int compute(int value)'], MAX_FUNCTION_LENGTH + 1)
        long_functions = check_lines(code)
        self.assertEqual(len(long_functions), 1)

    def test_function_at_limit_is_not_reported(self):
        code = build_function(['int compute(int value)'], MAX_FUNCTION_LENGTH - 3)
        self.assertEqual(check_lines(code), [])


if __name__ == "__main__":
    unittest.main()