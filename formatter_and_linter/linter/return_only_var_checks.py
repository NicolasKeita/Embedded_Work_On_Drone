#!/usr/bin/env python3
"""
Return-Only Variable Checks

Detects local variables that are declared/initialized and then immediately
returned via 'return var;' as their first following executable statement,
ignoring blank lines and comments in between. Any interleaved executable
instruction, a return of a modified expression (member access, call, ...),
or a return of a different identifier cancels the detection.

The declaration may span several lines (braced initializer, designated
initializers, parenthesized constructor call or copy initialization). The
reported line number is the one where the declaration starts.
"""

import re
from typing import List, Optional, Tuple

from linter.style_checks import _mask_strings_and_comments, _NON_DECLARATION_KEYWORD_RE
from linter.multiple_var_decl_checks import _has_top_level_declaration_comma

RETURN_ONLY_VAR_MESSAGE = (
    "Variable '{name}' déclarée uniquement pour être retournée immédiatement. "
    "Préférer un 'return' direct de l'expression."
)

_DECLARATION_QUALIFIERS_RE = (
    r'(?:(?:const|constexpr|consteval|constinit|static|inline|extern|mutable|'
    r'volatile|unsigned|signed|short|long|thread_local)\s+)*'
)

_DECLARATION_TYPE_NAME_RE = r'[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*'

_DECLARATION_TEMPLATE_ARGS_RE = r'(?:\s*<(?:[^<>]|<(?:[^<>]|<[^<>]*>)*>)*>\s*)?'

_DECLARATION_POINTER_REFS_RE = r'(?:\s*[*&]\s*)*'

_DECLARATOR_NAME_RE = r'(?P<name>[A-Za-z_]\w*)'

_INITIALIZED_DECL_RE = re.compile(
    rf'^{_DECLARATION_QUALIFIERS_RE}'
    rf'(?:auto\s+|{_DECLARATION_TYPE_NAME_RE}{_DECLARATION_TEMPLATE_ARGS_RE})'
    rf'{_DECLARATION_POINTER_REFS_RE}'
    rf'\s*{_DECLARATOR_NAME_RE}\s*'
    rf'(?:\{{|\(|=)'
)

_RETURN_VAR_RE = re.compile(r'^return\s+(?P<name>[A-Za-z_]\w*)\s*;\s*$')


def format_return_only_var_message(variable_name: str) -> str:
    """
    Render the warning message for a variable declared only to be returned
    immediately.
    """
    return RETURN_ONLY_VAR_MESSAGE.format(name=variable_name)


def _is_blank_or_comment(stripped_masked: str) -> bool:
    """
    Whether a masked, stripped line should be skipped (empty or comment).
    """
    return not stripped_masked or stripped_masked.startswith('//') or stripped_masked.startswith('/*')


def _top_level_semicolon_index(stripped_masked: str) -> Optional[int]:
    """
    Return the index of the first top-level ';' (outside parentheses and
    outside the braced initializer) in a masked line, or None when absent.
    """
    paren_depth = 0
    brace_depth = 0
    length = len(stripped_masked)
    i = 0

    while i < length:
        char = stripped_masked[i]
        if char == '(':
            paren_depth += 1
        elif char == ')':
            if paren_depth > 0:
                paren_depth -= 1
        elif char == '{':
            brace_depth += 1
        elif char == '}':
            if brace_depth > 0:
                brace_depth -= 1
        elif char == ';' and paren_depth == 0 and brace_depth == 0:
            return i
        i += 1

    return None


def _declaration_ends_on_line(stripped_masked: str) -> bool:
    """
    Whether a declaration statement terminates on this masked line.

    A declaration ends at the first top-level ';' that is outside parentheses
    and outside the braced/parenthesized initializer. When the initializer is
    a copy initialization ('name = expr;'), the terminating ';' is top-level.
    """
    return _top_level_semicolon_index(stripped_masked) is not None


def _is_single_complete_declaration(stripped_masked: str) -> bool:
    """
    Whether a masked, stripped line is a single complete initialized
    declaration: it terminates with a top-level ';', nothing executable follows
    that ';' on the same line, and there is no top-level comma (which would
    indicate multiple declarators such as 'auto a = 1, b = 2;').
    """
    semi_index = _top_level_semicolon_index(stripped_masked)
    if semi_index is None:
        return False

    if stripped_masked[semi_index + 1:].strip() != '':
        return False

    if _has_top_level_declaration_comma(stripped_masked[:semi_index + 1]):
        return False

    return True



def _parse_initialized_declaration(stripped_masked: str) -> Optional[str]:
    """
    Return the declared variable name when the masked statement starts a
    local variable declaration with initialization (braced, parenthesized or
    copy initialization), or None otherwise.

    The statement does not need to be complete on a single line: only the
    declarator prefix is matched here, multi-line completion is handled by the
    caller.
    """
    if _NON_DECLARATION_KEYWORD_RE.match(stripped_masked):
        return None

    match = _INITIALIZED_DECL_RE.match(stripped_masked)
    if match is None:
        return None

    return match.group('name')


def _declaration_continues_on_line(stripped_masked: str) -> bool:
    """
    Whether a masked, stripped line is a plausible continuation of an
    in-progress multi-line declaration (part of its initializer).

    A line that terminates the declaration (top-level ';') is handled
    separately by _declaration_ends_on_line. A standalone scope brace at
    top level (e.g. a closing '}' that is not part of the initializer) is not
    a continuation and cancels the pending declaration.
    """
    if stripped_masked in ('{', '}'):
        return False
    return True


def _find_declaration_end(
    lines: List[str],
    start_index: int,
    in_block_comment: bool,
) -> Tuple[Optional[int], bool]:
    """
    Starting at start_index (a line that begins an initialized declaration),
    scan forward until the declaration terminates (top-level ';' outside the
    initializer). Return (end_index, in_block_comment) where end_index is the
    0-based index of the terminating line, or (None, in_block_comment) when the
    declaration never terminates cleanly (scope closed or end of file).
    """
    index = start_index
    block_comment = in_block_comment

    while index < len(lines):
        masked, block_comment = _mask_strings_and_comments(lines[index], block_comment)
        stripped = masked.strip()

        if _is_blank_or_comment(stripped) or stripped.startswith('#'):
            index += 1
            continue

        if _is_single_complete_declaration(stripped):
            return index, block_comment

        if _declaration_ends_on_line(stripped):
            return None, block_comment

        if not _declaration_continues_on_line(stripped):
            return None, block_comment

        index += 1

    return None, block_comment



def check_return_only_variable(code: str) -> List[Tuple[int, str]]:
    """
    Report local variables declared/initialized and then immediately returned
    via 'return var;' as their first following executable statement (blank
    lines and comments ignored), as (declaration_line, variable_name) tuples
    with 1-based line numbers.

    Any interleaved executable instruction, a return of a modified expression
    (member access, function call, arithmetic, ...) or a return of a different
    identifier cancels the detection. A pending declaration that is not
    followed by a matching return is discarded as soon as another executable
    statement appears.
    """
    lines = code.splitlines()
    violations: List[Tuple[int, str]] = []
    in_block_comment = False
    pending_name: Optional[str] = None
    pending_line = 0

    line_index = 0
    while line_index < len(lines):
        raw_line = lines[line_index]
        masked_line, in_block_comment = _mask_strings_and_comments(raw_line, in_block_comment)
        stripped = masked_line.strip()

        if _is_blank_or_comment(stripped) or stripped.startswith('#'):
            line_index += 1
            continue

        if pending_name is not None:
            return_match = _RETURN_VAR_RE.match(stripped)
            if return_match is not None and return_match.group('name') == pending_name:
                violations.append((pending_line, pending_name))
                pending_name = None
                line_index += 1
                continue

            pending_name = None
            declared_name = _parse_initialized_declaration(stripped)
            if declared_name is not None and _is_single_complete_declaration(stripped):
                pending_name = declared_name
                pending_line = line_index + 1
                line_index += 1
                continue
            line_index += 1
            continue

        declared_name = _parse_initialized_declaration(stripped)
        if declared_name is None:
            line_index += 1
            continue

        decl_start = line_index
        if _is_single_complete_declaration(stripped):
            pending_name = declared_name
            pending_line = decl_start + 1
            line_index += 1
            continue

        if _declaration_ends_on_line(stripped):
            line_index += 1
            continue

        end_index, in_block_comment = _find_declaration_end(lines, decl_start, in_block_comment)
        if end_index is None:
            line_index += 1
            continue

        pending_name = declared_name
        pending_line = decl_start + 1
        line_index = end_index + 1

    return violations


