#!/usr/bin/env python3
"""
C++20 Designated Initializer Tests

Unit tests (pytest style) for the empty-brace declaration check: a variable
declared with an empty-brace value initialization whose first following
executable statement (blank lines and comments ignored) assigns one of its
members is reported with a C++20 designated initializer suggestion. Consecutive
assignments on the same variable are grouped into a single suggestion.
"""

import contextlib
import io
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.designated_init_checks import (
    check_designated_init_candidates,
    format_designated_init_message,
)
from linter.linter import lint_code


def check_code(code):
    return check_designated_init_candidates('\n'.join(code))


# --- Required cases -------------------------------------------------------

def test_empty_braces_then_member_assignment_is_reported():
    code = [
        'TypeVariable nom_variable{};',
        'nom_variable.nom_champ = valeur;',
    ]
    violations = check_code(code)
    assert violations == [(1, 'TypeVariable', 'nom_variable', ['nom_champ'])]


def test_empty_braces_with_space_then_member_assignment_is_reported():
    code = [
        'TypeVariable nom_variable {};',
        'nom_variable.nom_champ = valeur;',
    ]
    violations = check_code(code)
    assert violations == [(1, 'TypeVariable', 'nom_variable', ['nom_champ'])]


def test_extra_spaces_inside_braces_are_accepted():
    code = [
        'Type v { } ;',
        'v.champ = 1;',
    ]
    assert check_code(code) == [(1, 'Type', 'v', ['champ'])]


# --- Multi-field grouping -------------------------------------------------

def test_consecutive_member_assignments_are_grouped():
    code = [
        'Summary summary{};',
        'summary.a = 1;',
        'summary.b = 2;',
    ]
    violations = check_code(code)
    assert violations == [(1, 'Summary', 'summary', ['a', 'b'])]


def test_interleaved_statement_stops_grouping_but_reports_first_field():
    code = [
        'Summary summary{};',
        'summary.a = 1;',
        'do_something_else();',
        'summary.b = 2;',
    ]
    assert check_code(code) == [(1, 'Summary', 'summary', ['a'])]


def test_assignment_to_other_variable_stops_grouping():
    code = [
        'Summary summary{};',
        'summary.a = 1;',
        'other.b = 2;',
        'summary.c = 3;',
    ]
    assert check_code(code) == [(1, 'Summary', 'summary', ['a'])]


# --- Blank lines / comments are ignored ------------------------------------

def test_blank_lines_and_comments_are_ignored():
    code = [
        'SilEvent ev{};',
        '',
        '// Initialisation du contexte',
        'ev.timestamp = ctx.time;',
    ]
    assert check_code(code) == [(1, 'SilEvent', 'ev', ['timestamp'])]


def test_block_comment_is_ignored():
    code = [
        'SilEvent ev{};',
        '/* Initialization',
        '   spans lines */',
        'ev.timestamp = ctx.time;',
    ]
    assert check_code(code) == [(1, 'SilEvent', 'ev', ['timestamp'])]


def test_trailing_comment_on_declaration_is_ignored():
    code = [
        'Type v{}; // the variable under test',
        'v.champ = 1;',
    ]
    assert check_code(code) == [(1, 'Type', 'v', ['champ'])]


# --- Exclusions ------------------------------------------------------------

def test_non_empty_braced_initializer_is_not_reported():
    code = [
        'Type v{123};',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_designated_initializer_is_not_reported():
    code = [
        'Type v{.champ = 1};',
        'v.other = 2;',
    ]
    assert check_code(code) == []


def test_copy_initialization_is_not_reported():
    code = [
        'Type v = {};',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_compound_assignment_cancels_detection():
    code = [
        'Type v{};',
        'v.champ += 1;',
    ]
    assert check_code(code) == []


def test_comparison_cancels_detection():
    code = [
        'Type v{};',
        'v.champ == valeur;',
    ]
    assert check_code(code) == []


def test_rhs_reading_declared_variable_cancels_detection():
    code = [
        'Type v{};',
        'v.y = v.x + 1;',
    ]
    assert check_code(code) == []


def test_rhs_passing_declared_variable_cancels_detection():
    code = [
        'Type v{};',
        'v.y = helper(v);',
    ]
    assert check_code(code) == []


def test_interleaved_read_before_first_assignment_cancels():
    code = [
        'Type v{};',
        'use(v);',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_interleaved_call_before_first_assignment_cancels():
    code = [
        'Type v{};',
        'do_something();',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_opening_scope_brace_cancels():
    code = [
        'Type v{};',
        '{',
        '    v.champ = 1;',
        '}',
    ]
    assert check_code(code) == []


def test_closing_scope_brace_cancels():
    code = [
        'void f()',
        '{',
        '    Type v{};',
        '}',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


# --- False positive guards ------------------------------------------------

def test_multiple_declarators_are_not_reported():
    code = [
        'Type a{}, b{};',
        'a.champ = 1;',
    ]
    assert check_code(code) == []


def test_two_statements_on_one_line_are_not_reported():
    code = [
        'Type v{}; prepare();',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_return_keyword_line_is_not_a_declaration():
    code = [
        'return v{};',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_function_declaration_is_not_reported():
    code = [
        'Type v();',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


def test_string_content_is_ignored():
    code = [
        'Type v{};',
        'log("v.champ = 1");',
        'v.champ = 1;',
    ]
    assert check_code(code) == []


# --- Additional detection coverage ----------------------------------------

def test_template_type_declaration_is_reported():
    code = [
        'std::map<std::uint32_t, std::string> lookup{};',
        'lookup.index = 1;',
    ]
    assert check_code(code) == [(1, 'std::map<std::uint32_t, std::string>', 'lookup', ['index'])]


def test_qualified_declaration_type_is_kept_in_suggestion():
    code = [
        'static const Config* cfg{};',
        'cfg.frequency = 2.0;',
    ]
    assert check_code(code) == [(1, 'static const Config*', 'cfg', ['frequency'])]


def test_namespaced_type_declaration_is_reported():
    code = [
        'sim::control::TargetState target{};',
        'target.altitude = 10.0;',
    ]
    assert check_code(code) == [(1, 'sim::control::TargetState', 'target', ['altitude'])]


def test_nested_member_chain_is_reported():
    code = [
        'Type v{};',
        'v.sub.field = value;',
    ]
    assert check_code(code) == [(1, 'Type', 'v', ['sub.field'])]


def test_detection_inside_function_body():
    code = [
        'void run(const Context& ctx)',
        '{',
        '    SilEvent ev{};',
        '',
        '    // Initialisation du contexte',
        '    ev.timestamp = ctx.time;',
        '}',
    ]
    assert check_code(code) == [(3, 'SilEvent', 'ev', ['timestamp'])]


def test_multiple_violations_are_reported():
    code = [
        'Type first{};',
        'first.a = 1;',
        'Type second{};',
        'second.b = 2;',
    ]
    violations = check_code(code)
    assert violations == [
        (1, 'Type', 'first', ['a']),
        (3, 'Type', 'second', ['b']),
    ]


def test_pending_is_replaced_by_following_declaration():
    code = [
        'Type first{};',
        'Type second{};',
        'second.b = 2;',
    ]
    assert check_code(code) == [(2, 'Type', 'second', ['b'])]


# --- Message formatting ----------------------------------------------------

def test_message_text():
    assert format_designated_init_message('TypeVariable', 'nom_variable', ['nom_champ']) == (
        "Warning [C++20-designated-init]: Préférez l'initialisation désignée "
        "'TypeVariable nom_variable{.nom_champ = ...};' plutôt qu'une initialisation "
        "vide suivie d'une affectation."
    )


def test_multi_field_message_text():
    assert format_designated_init_message('Summary', 'summary', ['a', 'b']) == (
        "Warning [C++20-designated-init]: Préférez l'initialisation désignée "
        "'Summary summary{.a = ..., .b = ...};' plutôt qu'une initialisation "
        "vide suivie d'une affectation."
    )


def test_nested_field_message_text():
    assert format_designated_init_message('Type', 'v', ['sub.field']) == (
        "Warning [C++20-designated-init]: Préférez l'initialisation désignée "
        "'Type v{.sub.field = ...};' plutôt qu'une initialisation vide suivie d'une "
        "affectation."
    )


# --- End-to-end integration ------------------------------------------------

def test_lint_code_flags_designated_init_warning():
    code = (
        'void run()\n'
        '{\n'
        '    TypeVariable nom_variable{};\n'
        '    nom_variable.nom_champ = valeur;\n'
        '\n'
        '}\n'
    )
    capture = io.StringIO()
    with contextlib.redirect_stderr(capture):
        has_issues = lint_code(code)
    assert has_issues
    assert '[C++20-designated-init]' in capture.getvalue()
    assert 'TypeVariable nom_variable{.nom_champ = ...};' in capture.getvalue()


def test_lint_code_is_clean_without_designated_init_pattern():
    code = (
        'void run()\n'
        '{\n'
        '    TypeVariable nom_variable{.nom_champ = valeur};\n'
        '\n'
        '}\n'
    )
    capture = io.StringIO()
    with contextlib.redirect_stderr(capture):
        has_issues = lint_code(code)
    assert not has_issues
    assert '[C++20-designated-init]' not in capture.getvalue()


if __name__ == '__main__':
    import unittest

    _globals = globals()
    _cases = [name for name in _globals if name.startswith('test_')]

    class _Wrapper(unittest.TestCase):
        pass

    for _name in _cases:
        setattr(_Wrapper, _name, staticmethod(_globals[_name]))

    unittest.main()
