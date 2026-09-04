#!/usr/bin/env python3
"""
Uninitialized Declaration Tests

Unit tests for the declaration-then-member-assignment check: a variable
declared without initialization whose first following executable statement
(blank lines and comments ignored) assigns one of its members is reported.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.uninitialized_decl_checks import (
    check_uninitialized_declarations,
    format_uninitialized_decl_message,
)


def check_code(code):
    return check_uninitialized_declarations('\n'.join(code))


class TestUninitializedDeclarations(unittest.TestCase):

    def test_simple_declaration_then_member_assignment_is_reported(self):
        code = [
            'TypeVar ma_var;',
            'ma_var.champ1 = val1;',
            'ma_var.champ2 = val2;',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 1)
        self.assertEqual(violations[0][1], 'ma_var')

    def test_blank_lines_and_comments_are_ignored(self):
        code = [
            'SilEvent injected;',
            '',
            '// Initialisation du contexte',
            'injected.timestamp = ctx.time;',
            'injected.source = "ENV";',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 1)
        self.assertEqual(violations[0][1], 'injected')

    def test_block_comment_is_ignored(self):
        code = [
            'SilEvent injected;',
            '/* Initialization of the context',
            '   spans several lines */',
            'injected.timestamp = ctx.time;',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 1)
        self.assertEqual(violations[0][1], 'injected')

    def test_trailing_comment_on_declaration_is_ignored(self):
        code = [
            'TypeVar ma_var; // the variable under test',
            'ma_var.champ1 = val1;',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 1)

    def test_interleaved_executable_code_cancels_detection(self):
        code = [
            'TypeVar ma_var;',
            'do_something();',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_assignment_to_other_variable_cancels_detection(self):
        code = [
            'TypeVar ma_var;',
            'other.champ1 = val1;',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_comparison_cancels_detection(self):
        code = [
            'TypeVar ma_var;',
            'ma_var.champ1 == val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_compound_assignment_cancels_detection(self):
        code = [
            'TypeVar ma_var;',
            'ma_var.champ1 += val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_direct_brace_initialization_is_not_reported(self):
        code = [
            'TypeVar ma_var{};',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_designated_initializer_is_not_reported(self):
        code = [
            'TypeVar ma_var{.champ1 = val1};',
            'ma_var.champ2 = val2;',
        ]
        self.assertEqual(check_code(code), [])

    def test_copy_initialization_is_not_reported(self):
        code = [
            'TypeVar ma_var = make_var();',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_parenthesized_initialization_is_not_reported(self):
        code = [
            'TypeVar ma_var(3);',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_struct_member_declarations_are_not_reported(self):
        code = [
            'struct Config {',
            '    std::uint32_t count;',
            '',
            '    // Ratio comment',
            '    std::float64_t ratio;',
            '};',
        ]
        self.assertEqual(check_code(code), [])

    def test_function_signature_is_not_reported(self):
        code = [
            'void process(double input);',
            'process(1.0);',
        ]
        self.assertEqual(check_code(code), [])

    def test_member_function_after_member_declaration_is_not_reported(self):
        code = [
            'struct Sensor {',
            '    Calib calib;',
            '',
            '    void reset()',
            '    {',
            '        calib.scale = 1.0f;',
            '    }',
            '};',
        ]
        self.assertEqual(check_code(code), [])

    def test_detection_inside_function_body(self):
        code = [
            'void run(const Context& ctx)',
            '{',
            '    SilEvent injected;',
            '',
            '    // Initialisation du contexte',
            '    injected.timestamp = ctx.time;',
            '    injected.source = "ENV";',
            '}',
        ]
        violations = check_code(code)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], 3)
        self.assertEqual(violations[0][1], 'injected')

    def test_closing_scope_cancels_detection(self):
        code = [
            'void f()',
            '{',
            '    if (cond)',
            '    {',
            '        Type ma_var;',
            '    }',
            '    ma_var.champ1 = val1;',
            '}',
        ]
        self.assertEqual(check_code(code), [])

    def test_keyword_statement_cancels_detection(self):
        code = [
            'TypeVar ma_var;',
            'return;',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_multiple_violations_are_reported(self):
        code = [
            'Type first;',
            'first.a = 1;',
            'Type second;',
            'second.b = 2;',
        ]
        self.assertEqual(check_code(code), [(1, 'first'), (3, 'second')])

    def test_pending_is_replaced_by_following_declaration(self):
        code = [
            'Type first;',
            'Type second;',
            'second.b = 2;',
        ]
        self.assertEqual(check_code(code), [(2, 'second')])

    def test_template_type_declaration_is_reported(self):
        code = [
            'std::map<std::uint32_t, std::string> lookup;',
            'lookup.index = 1;',
        ]
        self.assertEqual(check_code(code), [(1, 'lookup')])

    def test_nested_template_type_declaration_is_reported(self):
        code = [
            'std::map<std::string, std::vector<int>> values;',
            'values.count = 1;',
        ]
        self.assertEqual(check_code(code), [(1, 'values')])

    def test_reference_declaration_is_reported(self):
        code = [
            'Config& config;',
            'config.count = 1;',
        ]
        self.assertEqual(check_code(code), [(1, 'config')])

    def test_multiple_declarators_are_not_reported(self):
        code = [
            'Type a, b;',
            'a.champ = 1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_two_statements_on_one_line_are_not_reported(self):
        code = [
            'TypeVar ma_var; prepare();',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_string_content_is_ignored(self):
        code = [
            'TypeVar ma_var;',
            'log("ma_var.champ1 = val1");',
            'ma_var.champ1 = val1;',
        ]
        self.assertEqual(check_code(code), [])

    def test_message_text(self):
        self.assertEqual(
            format_uninitialized_decl_message('injected'),
            "Variable 'injected' déclarée puis initialisée par assignation membre par membre. "
            "Préférer l'initialisation directe ou un designated initializer (C++20).",
        )


if __name__ == '__main__':
    unittest.main()