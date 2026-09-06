#!/usr/bin/env python3
"""
Multiple Variable Declaration Checks

Detects declarations of several variables on the same line, separated by
top-level commas (e.g. 'Type a, b;'). Commas inside template angle
brackets, function call parentheses or braced initializers are ignored.
"""

import re
from typing import List, Tuple

from linter.style_checks import _mask_strings_and_comments, _NON_DECLARATION_KEYWORD_RE

MULTIPLE_VAR_DECL_MESSAGE = "[MULTIPLE_VAR_DECL] Declare only one variable per line."

_DECLARED_IDENTIFIER_RE = re.compile(r'\s*[&*]*\s*[A-Za-z_]\w*\s*(?:=|;|,|\(|\[|\{|$)')


def _scan_line_for_top_level_comma(line: str, initial_paren_depth: int = 0) -> Tuple[bool, int]:
    """
    Check whether a masked code line contains a comma at declaration level
    (outside parentheses, template angle brackets and braced initializers)
    followed by something that looks like a declared identifier.

    The parenthesis depth starts at initial_paren_depth so that call
    arguments opened on previous lines (multi-line function calls) are not
    mistaken for top-level commas. Returns whether such a comma was found
    and the parenthesis depth reached at end of line, so callers can carry
    it over to continuation lines.
    """
    paren_depth = initial_paren_depth
    angle_depth = 0
    brace_depth = 0
    length = len(line)
    i = 0

    while i < length:
        char = line[i]
        if char == '(':
            paren_depth += 1
        elif char == ')':
            if paren_depth > 0:
                paren_depth -= 1
        elif char == '<':
            angle_depth += 1
        elif char == '>':
            if angle_depth > 0:
                angle_depth -= 1
        elif char == '{':
            brace_depth += 1
        elif char == '}':
            if brace_depth > 0:
                brace_depth -= 1
        elif char == ',' and paren_depth == 0 and angle_depth == 0 and brace_depth == 0:
            if _DECLARED_IDENTIFIER_RE.match(line[i + 1:]):
                return True, paren_depth
        i += 1

    return False, paren_depth


def _has_top_level_declaration_comma(line: str, initial_paren_depth: int = 0) -> bool:
    """
    Check whether a masked code line contains a comma at declaration level
    (outside parentheses, template angle brackets and braced initializers)
    followed by something that looks like a declared identifier.
    """
    has_top_level_comma, _ = _scan_line_for_top_level_comma(line, initial_paren_depth)
    return has_top_level_comma


def check_multiple_var_declarations(code: str) -> List[int]:
    """
    Report lines declaring several variables separated by top-level commas.

    Returns a list of 1-based line numbers. Strings, comments, template
    argument lists, function call arguments and braced initializer lists are
    excluded from detection. The parenthesis depth is carried across lines so
    that continuation lines of multi-line calls or parenthesized expressions
    are never mistaken for top-level declarations.
    """
    lines = code.splitlines()
    violations: List[int] = []
    in_block_comment = False
    paren_depth = 0

    for line_index, raw_line in enumerate(lines):
        masked_line, in_block_comment = _mask_strings_and_comments(raw_line, in_block_comment)
        stripped = masked_line.strip()
        if not stripped or stripped.startswith('#'):
            continue
        has_top_level_comma, paren_depth = _scan_line_for_top_level_comma(stripped, paren_depth)
        if _NON_DECLARATION_KEYWORD_RE.match(stripped):
            continue
        if not stripped.endswith(';'):
            continue
        if has_top_level_comma:
            violations.append(line_index + 1)

    return violations
