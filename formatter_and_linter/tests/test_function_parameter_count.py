#!/usr/bin/env python3
"""
Function Parameter Count Tests

Unit tests for the function-parameter-count linter check
(WARN_FUNCTION_TOO_MANY_PARAMETERS).

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.function_parameter_count_checks import (
    MAX_FUNCTION_PARAMETERS,
    WARN_FUNCTION_TOO_MANY_PARAMETERS_TAG,
    check_function_parameter_count,
    format_function_parameter_count_message,
)


def check_code(code):
    if isinstance(code, list):
        code = '\n'.join(code)
    return check_function_parameter_count(code)


class TestFunctionParameterCount(unittest.TestCase):

    def test_max_parameters_constant(self):
        self.assertEqual(MAX_FUNCTION_PARAMETERS, 5)

    def test_five_parameters_is_allowed(self):
        code = 'void foo(int a, int b, int c, int d, int e) { return; }'
        self.assertEqual(check_code(code), [])

    def test_six_parameters_is_reported(self):
        code = 'void foo(int a, int b, int c, int d, int e, int f) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        name, line, count = violations[0]
        self.assertEqual(name, 'foo')
        self.assertEqual(line, 1)
        self.assertEqual(count, 6)

    def test_empty_parameter_list_is_allowed(self):
        code = 'void foo() { return; }'
        self.assertEqual(check_code(code), [])

    def test_void_parameter_is_allowed(self):
        code = 'void foo(void) { return; }'
        self.assertEqual(check_code(code), [])

    def test_one_parameter_is_allowed(self):
        code = 'void foo(int a) { return; }'
        self.assertEqual(check_code(code), [])

    def test_function_pointer_parameter_is_counted_correctly(self):
        code = 'void foo(int a, void (*cb)(int, int), int b) { return; }'
        self.assertEqual(check_code(code), [])

    def test_function_pointer_six_params_is_reported(self):
        code = 'void foo(int a, void (*cb)(int, int), int b, int c, int d, int e) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 6)

    def test_template_parameter_is_counted_correctly(self):
        code = 'void foo(std::vector<int, Alloc> v, int a) { return; }'
        self.assertEqual(check_code(code), [])

    def test_nested_template_comma_is_not_counted(self):
        code = 'void foo(std::array<std::vector<int>, 3> a, int b) { return; }'

    def test_template_six_params_is_reported(self):
        code = 'void foo(std::map<int, int> a, int b, int c, int d, int e, int f) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 6)

    def test_multiline_signature_is_handled(self):
        code = [
            'void foo(',
            '    int a,',
            '    int b,',
            '    int c,',
            '    int d,',
            '    int e,',
            '    int f',
            ') {',
            '    return;',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'foo')
        self.assertEqual(violations[0][1], 1)
        self.assertEqual(violations[0][2], 6)

    def test_multiline_five_params_is_allowed(self):
        code = [
            'void foo(',
            '    int a,',
            '    int b,',
            '    int c,',
            '    int d,',
            '    int e',
            ') {',
            '    return;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_constructor_with_initializer_list(self):
        code = [
            'Foo::Foo(int a, int b, int c, int d, int e, int f) : x_(a), y_(b) {',
            '    init();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'Foo::Foo')
        self.assertEqual(violations[0][2], 6)

    def test_member_function_with_const_qualifier(self):
        code = 'void Foo::bar(int a, int b, int c, int d, int e, int f) const { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'Foo::bar')
        self.assertEqual(violations[0][2], 6)

    def test_member_function_five_params_is_allowed(self):
        code = 'void Foo::bar(int a, int b, int c, int d, int e) const { return; }'
        self.assertEqual(check_code(code), [])

    def test_function_inside_class_is_detected(self):
        code = [
            'class Foo {',
            'public:',
            '    void bar(int a, int b, int c, int d, int e, int f) { return; }',
            '};',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'bar')
        self.assertEqual(violations[0][1], 3)
        self.assertEqual(violations[0][2], 6)

    def test_default_parameters_are_counted(self):
        code = 'void foo(int a = 1, int b = 2, int c = 3, int d = 4, int e = 5, int f = 6) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 6)

    def test_variadic_is_counted_as_parameter(self):
        code = 'void foo(int a, int b, int c, int d, int e, ...) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 6)

    def test_lambda_is_not_flagged(self):
        code = 'auto f = [](int a, int b, int c, int d, int e, int f) { return a; };'
        self.assertEqual(check_code(code), [])

    def test_function_call_is_not_flagged(self):
        code = [
            'void run() {',
            '    compute(a, b, c, d, e, f);',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_control_structures_are_not_flagged(self):
        code = [
            'int main() {',
            '    if (a > 0) { return 1; }',
            '    for (int i = 0; i < 10; ++i) { step(); }',
            '    while (x > 0) { --x; }',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_braced_default_value_commas_are_not_counted(self):
        code = 'void foo(int a = {1, 2, 3}, int b, int c, int d, int e) { return; }'
        self.assertEqual(check_code(code), [])

    def test_braced_default_six_params_is_reported(self):
        code = 'void foo(int a = {1, 2}, int b, int c, int d, int e, int f) { return; }'
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 6)

    def test_array_parameter_brackets_are_handled(self):
        code = 'void foo(int a[], int b, int c, int d, int e) { return; }'
        self.assertEqual(check_code(code), [])

    def test_template_function_is_detected(self):
        code = [
            'template<typename Target>',
            'bool parse_option(CliOptions& opts, int argc, char* argv[], int& idx,',
            '                  Target& field, std::string_view label) {',
            '    return true;',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'parse_option')
        self.assertEqual(violations[0][2], 6)

    def test_multiple_violations_are_reported_in_line_order(self):
        code = [
            'void first(int a, int b, int c, int d, int e, int f) { return; }',
            'void second(int a, int b, int c, int d, int e, int f) { return; }',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 2)
        self.assertEqual(violations[0][0], 'first')
        self.assertEqual(violations[0][1], 1)
        self.assertEqual(violations[1][0], 'second')
        self.assertEqual(violations[1][1], 2)

    def test_declaration_is_not_flagged(self):
        code = 'void foo(int a, int b, int c, int d, int e, int f);'
        self.assertEqual(check_code(code), [])

    def test_comment_commas_are_not_counted(self):
        code = [
            '/* void old(int a, int b, int c, int d, int e, int f) { return; } */',
            'void foo() { return; }',
        ]
        self.assertEqual(check_code(code), [])

    def test_message_format_matches_specification(self):
        message = format_function_parameter_count_message(
            'Src/App/Application.cpp', 'Run', 42, 6
        )
        self.assertEqual(
            message,
            '[WARN_FUNCTION_TOO_MANY_PARAMETERS] Src/App/Application.cpp:42 : '
            "la fonction 'Run' possède 6 paramètres (max : 5). "
            'Envisagez de regrouper certains paramètres dans une structure ou classe.',
        )

    def test_warning_tag_constant(self):
        self.assertEqual(
            WARN_FUNCTION_TOO_MANY_PARAMETERS_TAG,
            '[WARN_FUNCTION_TOO_MANY_PARAMETERS]',
        )


if __name__ == '__main__':
    unittest.main()

