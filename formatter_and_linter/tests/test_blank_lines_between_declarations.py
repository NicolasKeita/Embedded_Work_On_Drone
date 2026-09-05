#!/usr/bin/env python3
"""
Blank Lines Between Declarations Tests

Unit tests for removing blank lines between consecutive variable declarations
located at the beginning of C++ function bodies.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.declaration_blank_line_formatter import remove_blank_lines_between_declarations


def fmt(code):
    return remove_blank_lines_between_declarations('\n'.join(code))


class TestRemoveBlankLinesBetweenDeclarations(unittest.TestCase):

    def test_prompt_example_same_line_brace(self):
        code = [
            'void generate_faults() {',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '',
            '    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);',
            '',
            '    std::uniform_real_distribution<std::float64_t> duration_window(1.0, 8.0);',
            '',
            '    const std::uint64_t fault_roll = generator() % 6;',
            '',
            '    SimulationResult result{.sil_result = sil_result, .failure_reason = FailureReason::None};',
            '',
            '    const std::float64_t dx = sil_result.final_x_m - 0.0;',
            '',
            '    apply_fault(fault_roll);',
            '',
            '    if (dx > 0) {',
            '',
            '        do_something();',
            '    }',
            '}',
        ]
        expected = [
            'void generate_faults() {',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);',
            '    std::uniform_real_distribution<std::float64_t> duration_window(1.0, 8.0);',
            '    const std::uint64_t fault_roll = generator() % 6;',
            '    SimulationResult result{.sil_result = sil_result, .failure_reason = FailureReason::None};',
            '    const std::float64_t dx = sil_result.final_x_m - 0.0;',
            '',
            '    apply_fault(fault_roll);',
            '',
            '    if (dx > 0) {',
            '',
            '        do_something();',
            '    }',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_prompt_example_own_line_brace(self):
        code = [
            'void generate_faults()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '',
            '    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);',
            '',
            '    const std::uint64_t fault_roll = generator() % 6;',
            '',
            '    apply_fault(fault_roll);',
            '',
            '    if (dx > 0) {',
            '',
            '        do_something();',
            '    }',
            '}',
        ]
        expected = [
            'void generate_faults()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);',
            '    const std::uint64_t fault_roll = generator() % 6;',
            '',
            '    apply_fault(fault_roll);',
            '',
            '    if (dx > 0) {',
            '',
            '        do_something();',
            '    }',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_no_blank_lines_is_unchanged(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_single_declaration_with_blank_after_unchanged(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_reassignment_ends_zone(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    a = compute(a);',
            '    doSomething();',
            '}',
        ]
        expected = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    a = compute(a);',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_compound_assignment_not_declaration(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    state_.x += get_value();',
            '    state_.y -= 5;',
            '}',
        ]
        expected = [
            'void process()',
            '{',
            '    int a = 1;',
            '',
            '    state_.x += get_value();',
            '    state_.y -= 5;',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_stream_operator_not_declaration(self):
        code = [
            'void log()',
            '{',
            '    std::cout << "hello";',
            '',
            '    std::cout << "world";',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_return_statement_not_declaration(self):
        code = [
            'int compute()',
            '{',
            '    int a = 1;',
            '',
            '    int b = 2;',
            '',
            '    return a + b;',
            '}',
        ]
        expected = [
            'int compute()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    return a + b;',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_designated_initialiser_declaration(self):
        code = [
            'void build()',
            '{',
            '    ControlCommand cmd{.throttle = 1, .pitch = 2};',
            '',
            '    ControlCommand other{.throttle = 0};',
            '',
            '    dispatch(cmd);',
            '}',
        ]
        expected = [
            'void build()',
            '{',
            '    ControlCommand cmd{.throttle = 1, .pitch = 2};',
            '    ControlCommand other{.throttle = 0};',
            '',
            '    dispatch(cmd);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_template_with_constructor_call(self):
        code = [
            'void sample()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '',
            '    std::uniform_real_distribution<std::float64_t> window(2.0, 12.0);',
            '',
            '    const double r = unit(generator);',
            '',
            '    apply(r);',
            '}',
        ]
        expected = [
            'void sample()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '    std::uniform_real_distribution<std::float64_t> window(2.0, 12.0);',
            '    const double r = unit(generator);',
            '',
            '    apply(r);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_control_structure_blank_lines_preserved(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    if (a > 0) {',
            '',
            '        use(b);',
            '    }',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_multiple_functions_in_file(self):
        code = [
            'void first()',
            '{',
            '    int a = 1;',
            '',
            '    int b = 2;',
            '',
            '    run(a, b);',
            '}',
            '',
            'void second()',
            '{',
            '    int c = 3;',
            '    doMore(c);',
            '}',
        ]
        expected = [
            'void first()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    run(a, b);',
            '}',
            '',
            'void second()',
            '{',
            '    int c = 3;',
            '    doMore(c);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_namespace_body_not_compacted(self):
        code = [
            'namespace sim {',
            'int x = 1;',
            '',
            'int y = 2;',
            'doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_struct_body_not_compacted(self):
        code = [
            'struct Config',
            '{',
            '    int x = 1;',
            '',
            '    int y = 2;',
            '};',
        ]
        self.assertEqual(fmt(code), '\n'.join(code))

    def test_lambda_not_treated_as_function(self):
        code = [
            'void pass()',
            '{',
            '    auto lam = [](int v) { return v + 1; };',
            '',
            '    const int r = lam(2);',
            '',
            '    consume(r);',
            '}',
        ]
        expected = [
            'void pass()',
            '{',
            '    auto lam = [](int v) { return v + 1; };',
            '    const int r = lam(2);',
            '',
            '    consume(r);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_multiline_declaration_continuation(self):
        code = [
            'void init()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(',
            '        0.0, 1.0);',
            '',
            '    const double r = unit(generator);',
            '',
            '    apply(r);',
            '}',
        ]
        expected = [
            'void init()',
            '{',
            '    std::uniform_real_distribution<std::float64_t> unit(',
            '        0.0, 1.0);',
            '    const double r = unit(generator);',
            '',
            '    apply(r);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_aggregate_initialiser_spanning_lines(self):
        code = [
            'void init()',
            '{',
            '    Point p = {',
            '        1,',
            '        2,',
            '    };',
            '',
            '    const int total = p.x + p.y;',
            '',
            '    use(total);',
            '}',
        ]
        expected = [
            'void init()',
            '{',
            '    Point p = {',
            '        1,',
            '        2,',
            '    };',
            '    const int total = p.x + p.y;',
            '',
            '    use(total);',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_whitespace_only_blank_lines_count_as_blank(self):
        code = [
            'void process()',
            '{',
            '    int a = 1;',
            '   ',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        expected = [
            'void process()',
            '{',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))

    def test_trailing_newline_preserved(self):
        code = 'void f()\n{\n    int a = 1;\n\n    int b = 2;\n\n    use(a, b);\n}\n'
        expected = 'void f()\n{\n    int a = 1;\n    int b = 2;\n\n    use(a, b);\n}\n'
        self.assertEqual(remove_blank_lines_between_declarations(code), expected)

    def test_idempotent(self):
        code = [
            'void generate_faults() {',
            '    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);',
            '',
            '    std::uniform_real_distribution<std::float64_t> window(2.0, 12.0);',
            '',
            '    const std::uint64_t roll = generator() % 6;',
            '',
            '    apply(roll);',
            '}',
        ]
        once = fmt(code)
        self.assertEqual(fmt(once.splitlines()), once)

    def test_empty_input(self):
        self.assertEqual(remove_blank_lines_between_declarations(''), '')

    def test_leading_blank_after_brace_preserved(self):
        code = [
            'void process()',
            '{',
            '',
            '    int a = 1;',
            '',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        expected = [
            'void process()',
            '{',
            '',
            '    int a = 1;',
            '    int b = 2;',
            '',
            '    doSomething();',
            '}',
        ]
        self.assertEqual(fmt(code), '\n'.join(expected))


if __name__ == '__main__':
    unittest.main()
