#!/usr/bin/env python3
"""
Blank Line After Initialization Tests

Unit tests for the blank-line-after-initialization style check.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.style_checks import check_blank_line_after_initialization


def check_code(code: str):
    return check_blank_line_after_initialization('\n'.join(code))


class TestBlankLineAfterInitialization(unittest.TestCase):

    def test_missing_blank_line_is_reported(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'process')
        self.assertEqual(violations[0][2], 4)

    def test_present_blank_line_is_not_reported(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_single_declaration_without_blank_line(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 3)

    def test_no_declarations_no_violation(self):
        code = [
            'void process()',
            '{',
            '    doSomething();',
            '    doSomethingElse();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_declarations_then_return_without_blank(self):
        code = [
            'int compute()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '    return a + b;',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'compute')
        self.assertEqual(violations[0][2], 4)

    def test_declarations_then_return_with_blank(self):
        code = [
            'int compute()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    return a + b;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_mixed_declarations_and_assignments(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    a = compute(a);',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 3)

    def test_const_declarations(self):
        code = [
            'void process(double input)',
            '{',
            '    const int threshold = 10;',
            '    const double factor = input * 2.0;',
            '    processValue(threshold, factor);',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 4)

    def test_multiline_signature_with_violation(self):
        code = [
            'FlightController::Result run(',
            '    FlightController& ctrl,',
            '    Aircraft& aircraft)',
            '{',
            '    ControlCommand cmd;',
            '    cmd.wing_rpm = 100;',
            '    return Result::Success;',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'run')

    def test_multiline_signature_with_blank_line(self):
        code = [
            'FlightController::Result run(',
            '    FlightController& ctrl,',
            '    Aircraft& aircraft)',
            '{',
            '    ControlCommand cmd;',
            '',
            '    cmd.wing_rpm = 100;',
            '',
            '    return Result::Success;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_assignment_to_existing_var_is_not_declaration(self):
        code = [
            'void process()',
            '{',
            '    state_.mode = Mode::Auto;',
            '    cmd.wing_rpm = 100;',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_function_call_first_line_is_not_decl(self):
        code = [
            'void process()',
            '{',
            '    init();',
            '    int a = 1;',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_brace_on_same_line_with_body(self):
        code = [
            'void process() {',
            '    int a = 1;',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_empty_function_body(self):
        code = [
            'void process()',
            '{',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_declarations_at_end_of_function(self):
        code = [
            'void process()',
            '{',
            '    init();',
            '    int a = 1;',
            '    int b = 2;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_declarations_then_compound_statements(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    for (int i = 0; i < a; ++i) {',
            '        process();',
            '    }',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 3)

    def test_class_body_not_checked(self):
        code = [
            'class Data',
            '{',
            '    int x = 1;',
            '    int y = 2;',
            '};',
        ]
        self.assertEqual(check_code(code), [])

    def test_namespace_body_not_checked(self):
        code = [
            'namespace sim {',
            'int x = 1;',
            'int y = 2;',
            'doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_multiple_functions(self):
        code = [
            'void good()',
            '{',
            '    int a = 1;',
            '',
            '    doSomething();',
            '}',
            '',
            'void bad()',
            '{',
            '    int b = 2;',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 'bad')
        self.assertEqual(violations[0][2], 10)

    def test_stream_operator_not_declaration(self):
        code = [
            'void log()',
            '{',
            '    std::cout << "hello";',
            '    std::cout << "world";',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_brace_initialized_declarations(self):
        code = [
            'void process()',
            '{',
            '    ControlCommand cmd{1, 2, 3};',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 3)

    def test_if_block_not_checked_as_function(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    if (a > 0) {',
            '        int b = 2;',
            '        use(b);',
            '    }',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_comment_only_line_before_declarations(self):
        code = [
            'void process()',
            '{',
            '    // Initialize variables',
            '    int a = 1;',
            '    int b = 2;',
            '    doSomething();',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][2], 5)

    def test_string_with_semicolon_not_declared(self):
        code = [
            'void process()',
            '{',
            '    std::string msg = "hello;world";',
            '',
            '    log(msg);',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_string_with_brace_not_declared(self):
        code = [
            'void process()',
            '{',
            '    std::string msg = "hello { world";',
            '',
            '    log(msg);',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_compound_assignment_not_declaration(self):
        code = [
            'void process()',
            '{',
            '    state_.x += get_value();',
            '    state_.y -= 5;',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(check_code(code), [])


if __name__ == '__main__':
    unittest.main()
