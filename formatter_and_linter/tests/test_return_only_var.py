#!/usr/bin/env python3
"""
Return-Only Variable Tests

Unit tests (pytest style) for the declaration-then-immediate-return check:
a local variable declared/initialized and then returned via 'return var;' as
its first following executable statement (blank lines and comments ignored)
is reported. Any interleaved executable instruction, a return of a modified
expression, or a return of a different identifier cancels the detection.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.return_only_var_checks import (
    check_return_only_variable,
    format_return_only_var_message,
)


def check_code(code):
    return check_return_only_variable('\n'.join(code))


# --- Required cases -------------------------------------------------------

def test_single_line_declaration_then_return_is_reported():
    code = [
        'auto x = 1;',
        'return x;',
    ]
    violations = check_code(code)
    assert violations == [(1, 'x')]


def test_multiline_designated_initializers_with_comments_is_reported():
    code = [
        'const SensorValidity validity{',
        '    .altitude_valid = !std::isnan(telemetry.z),',
        '    .position_valid = horizontal <= limits.max_position_m',
        '};',
        '',
        '// Returns the computed validity',
        'return validity;',
    ]
    violations = check_code(code)
    assert violations == [(1, 'validity')]


def test_executable_code_between_declaration_and_return_is_not_reported():
    code = [
        'const auto status = evaluate();',
        'log_status(status);',
        'return status;',
    ]
    assert check_code(code) == []


# --- Additional detection coverage ---------------------------------------

def test_parenthesized_constructor_initialization_is_reported():
    code = [
        'auto result = compute_something(a, b);',
        'return result;',
    ]
    assert check_code(code) == [(1, 'result')]


def test_direct_constructor_call_initialization_is_reported():
    code = [
        'const Widget w(get_widget());',
        'return w;',
    ]
    assert check_code(code) == [(1, 'w')]


def test_const_qualified_declaration_is_reported():
    code = [
        'const auto value = compute();',
        'return value;',
    ]
    assert check_code(code) == [(1, 'value')]


def test_declaration_line_number_is_reported_not_return_line():
    code = [
        'void f()',
        '{',
        '    auto x = 1;',
        '    return x;',
        '}',
    ]
    assert check_code(code) == [(3, 'x')]


def test_multiline_parenthesized_initializer_is_reported():
    code = [
        'auto result = compute(',
        '    a,',
        '    b',
        ');',
        'return result;',
    ]
    assert check_code(code) == [(1, 'result')]


def test_blank_lines_between_declaration_and_return_are_ignored():
    code = [
        'auto x = 1;',
        '',
        '',
        'return x;',
    ]
    assert check_code(code) == [(1, 'x')]


def test_block_comment_between_declaration_and_return_is_ignored():
    code = [
        'auto x = 1;',
        '/* explain the return',
        '   on multiple lines */',
        'return x;',
    ]
    assert check_code(code) == [(1, 'x')]


def test_single_line_comment_between_declaration_and_return_is_ignored():
    code = [
        'auto x = 1;',
        '// Renvoie le statut calculé',
        'return x;',
    ]
    assert check_code(code) == [(1, 'x')]


def test_trailing_comment_on_declaration_is_ignored():
    code = [
        'auto x = 1; // the value',
        'return x;',
    ]
    assert check_code(code) == [(1, 'x')]


# --- Cancellation cases --------------------------------------------------

def test_return_of_member_access_is_not_reported():
    code = [
        'const auto status = evaluate();',
        'return status.is_valid;',
    ]
    assert check_code(code) == []


def test_return_of_method_call_is_not_reported():
    code = [
        'const auto status = evaluate();',
        'return status.to_string();',
    ]
    assert check_code(code) == []


def test_return_of_modified_expression_is_not_reported():
    code = [
        'const auto x = compute();',
        'return x + 1;',
    ]
    assert check_code(code) == []


def test_return_of_different_identifier_is_not_reported():
    code = [
        'auto x = compute();',
        'return y;',
    ]
    assert check_code(code) == []


def test_return_without_value_is_not_reported():
    code = [
        'auto x = compute();',
        'return;',
    ]
    assert check_code(code) == []


def test_assignment_between_declaration_and_return_is_not_reported():
    code = [
        'auto x = compute();',
        'x = other;',
        'return x;',
    ]
    assert check_code(code) == []


def test_call_between_declaration_and_return_is_not_reported():
    code = [
        'auto x = compute();',
        'log(x);',
        'return x;',
    ]
    assert check_code(code) == []


def test_direct_return_expression_is_not_reported():
    code = [
        'return SensorValidity{ .altitude_valid = true };',
    ]
    assert check_code(code) == []


def test_uninitialized_declaration_is_not_reported():
    code = [
        'Type x;',
        'return x;',
    ]
    assert check_code(code) == []


def test_multiple_declarators_are_not_reported():
    code = [
        'auto a = 1, b = 2;',
        'return a;',
    ]
    assert check_code(code) == []


def test_two_statements_on_one_line_are_not_reported():
    code = [
        'auto x = 1; prepare();',
        'return x;',
    ]
    assert check_code(code) == []


def test_string_content_is_ignored():
    code = [
        'auto x = 1;',
        'log("return x;");',
        'return x;',
    ]
    assert check_code(code) == []


def test_closing_scope_cancels_detection():
    code = [
        'void f()',
        '{',
        '    if (cond)',
        '    {',
        '        auto x = 1;',
        '    }',
        '    return x;',
        '}',
    ]
    assert check_code(code) == []


# --- Multiple violations & chaining --------------------------------------

def test_multiple_violations_are_reported():
    code = [
        'auto a = 1;',
        'return a;',
        'auto b = 2;',
        'return b;',
    ]
    assert check_code(code) == [(1, 'a'), (3, 'b')]


def test_new_declaration_replaces_pending_one():
    code = [
        'auto a = 1;',
        'auto b = 2;',
        'return b;',
    ]
    assert check_code(code) == [(2, 'b')]


def test_executable_statement_then_new_declaration_reports_only_second():
    code = [
        'auto a = 1;',
        'do_something(a);',
        'auto b = 2;',
        'return b;',
    ]
    assert check_code(code) == [(3, 'b')]


# --- Message formatting --------------------------------------------------

def test_message_text():
    assert format_return_only_var_message('validity') == (
        "Variable 'validity' déclarée uniquement pour être retournée immédiatement. "
        "Préférer un 'return' direct de l'expression."
    )


if __name__ == '__main__':
    import unittest

    _globals = globals()
    _cases = [name for name in _globals if name.startswith('test_')]

    class _Wrapper(unittest.TestCase):
        pass

    for _name in _cases:
        setattr(_Wrapper, _name, staticmethod(_globals[_name]))

    unittest.main()


