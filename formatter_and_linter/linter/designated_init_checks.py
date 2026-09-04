#!/usr/bin/env python3
"""
C++20 Designated Initializer Checks

Detects variables declared with empty-brace value initialization ('Type var{};'
or 'Type var {};') whose first following executable statement (blank lines and
comments ignored) assigns one of its members ('var.champ = value;'), and
suggests a C++20 designated initializer instead of the empty initialization
followed by a member-by-member assignment.

Consecutive member assignments on the same variable ('var.a = 1; var.b = 2;')
are grouped into a single suggestion 'Type var{.a = 1, .b = 2};'.

Non-empty initializer lists ('Type var{123};', 'Type var{.a = 1};'), copies and
parenthesized initializations are never reported. Any other executable
statement interleaved between the declaration and the first member assignment
(scope brace, read of the variable, parameter passing, ...) cancels the
detection, as does a member assignment whose right-hand side reads back the
declared variable.
"""

import re
from typing import List, Optional, Tuple

from linter.style_checks import _mask_strings_and_comments, _NON_DECLARATION_KEYWORD_RE

_DESIGNATED_INIT_MESSAGE = (
    "Warning [C++20-designated-init]: Préférez l'initialisation désignée "
    "'{suggestion}' plutôt qu'une initialisation vide suivie d'une affectation."
)

_DECLARATION_QUALIFIERS_RE = (
    r'(?:(?:const|constexpr|consteval|constinit|static|inline|extern|mutable|'
    r'volatile|unsigned|signed|short|long|thread_local)\s+)*'
)

_DECLARATION_TYPE_NAME_RE = r'[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*'

_DECLARATION_TEMPLATE_ARGS_RE = r'(?:\s*<(?:[^<>]|<(?:[^<>]|<[^<>]*>)*>)*>\s*)?'

_DECLARATION_POINTER_REFS_RE = r'(?:\s*[*&]\s*)*'

_DECLARATION_IDENTIFIER_RE = r'[A-Za-z_]\w*'

_EMPTY_BRACE_DECL_RE = re.compile(
    rf'^(?P<type>{_DECLARATION_QUALIFIERS_RE}'
    rf'{_DECLARATION_TYPE_NAME_RE}'
    rf'{_DECLARATION_TEMPLATE_ARGS_RE}'
    rf'{_DECLARATION_POINTER_REFS_RE})'
    rf'\s*(?P<name>{_DECLARATION_IDENTIFIER_RE})'
    r'\s*\{\s*\}\s*;\s*$'
)

_ASSIGN_OP_RE = r'\s*(?<![<>=!+\-*/%&|^])=(?!=)\s*'


def format_designated_init_message(
    type_name: str,
    variable_name: str,
    field_chains: List[str],
) -> str:
    """
    Render the warning message for an empty-brace initialization followed by
    member assignments. The suggestion lists every assigned field as a C++20
    designated initializer with '...' standing for the assigned value.
    """
    designators = ', '.join(f'.{field} = ...' for field in field_chains)
    suggestion = f"{type_name.strip()} {variable_name}{{{designators}}};"
    return _DESIGNATED_INIT_MESSAGE.format(suggestion=suggestion)


def _is_blank_or_comment(stripped_masked: str) -> bool:
    """
    Whether a masked, stripped line should be skipped (empty or a comment).
    """
    return not stripped_masked or stripped_masked.startswith('//') or stripped_masked.startswith('/*')


def _find_top_level_semicolon(stripped_masked: str) -> Optional[int]:
    """
    Return the index of the first top-level ';' (outside parentheses and
    braced initializers) in a masked line, or None when absent.
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


def _parse_empty_brace_declaration(stripped_masked: str) -> Optional[Tuple[str, str]]:
    """
    Return (type, variable_name) when the masked statement is a single
    declaration with an empty-brace value initialization ('Type var{};'),
    or None otherwise. Non-empty braced initializers, designated
    initializers, parenthesized/copy initializations, multiple declarators
    and multi-statement lines are rejected.
    """
    if not stripped_masked.endswith(';'):
        return None

    if _NON_DECLARATION_KEYWORD_RE.match(stripped_masked):
        return None

    match = _EMPTY_BRACE_DECL_RE.match(stripped_masked)
    if match is None:
        return None

    return match.group('type').strip(), match.group('name')


def _parse_member_assignment(
    stripped_masked: str,
    variable_name: str,
) -> Optional[Tuple[str, str]]:
    """
    Return (field_chain, rhs) when the masked, stripped line is a single
    plain assignment statement on a member of variable_name ('var.member = x;'
    or 'var.sub.member = x;'), or None otherwise. Compound assignments,
    comparisons, multiple statements on the line and trailing content after
    the terminating ';' are rejected.
    """
    semicolon_index = _find_top_level_semicolon(stripped_masked)
    if semicolon_index is None:
        return None

    if stripped_masked[semicolon_index + 1:].strip():
        return None

    head = stripped_masked[:semicolon_index]
    pattern = (
        rf'^(?P<lhs>{re.escape(variable_name)}'
        rf'(?:\s*\.\s*{_DECLARATION_IDENTIFIER_RE})+)'
        rf'{_ASSIGN_OP_RE}'
        rf'(?P<rhs>.+?)\s*$'
    )
    match = re.match(pattern, head)
    if match is None:
        return None

    lhs_body = match.group('lhs')[len(variable_name):]
    field_parts = [part.strip() for part in lhs_body.split('.')]
    field_chain = '.'.join(part for part in field_parts if part)
    if not field_chain:
        return None

    return field_chain, match.group('rhs')


def _references_variable(text: str, variable_name: str) -> bool:
    """
    Whether the masked text reads back the declared variable name as an
    identifier (word-boundary delimited, so 'var_count' does not count).
    """
    return re.search(rf'\b{re.escape(variable_name)}\b', text) is not None


def check_designated_init_candidates(
    code: str,
) -> List[Tuple[int, str, str, List[str]]]:
    """
    Report empty-brace declarations whose first following executable statement
    (blank lines and comments ignored) assigns a member of the declared
    variable, as (declaration_line, type, variable_name, field_chains) tuples
    with 1-based line numbers.

    Consecutive plain assignments on the same variable are gathered into a
    single violation. Any executable statement interleaved before the first
    member assignment, a right-hand side that reads back the declared variable,
    or a scope brace cancels the detection. When the gathered group is closed
    by a non-assignment statement (for example an unrelated call), the fields
    already collected are reported and the scan resumes after that statement.
    """
    lines = code.splitlines()
    violations: List[Tuple[int, str, str, List[str]]] = []
    in_block_comment = False
    line_index = 0

    while line_index < len(lines):
        masked_line, in_block_comment = _mask_strings_and_comments(lines[line_index], in_block_comment)
        stripped = masked_line.strip()

        if _is_blank_or_comment(stripped) or stripped.startswith('#'):
            line_index += 1
            continue

        declaration = _parse_empty_brace_declaration(stripped)
        if declaration is None:
            line_index += 1
            continue

        type_name, variable_name = declaration
        declaration_line = line_index + 1
        field_chains: List[str] = []

        scan_index = line_index + 1
        while scan_index < len(lines):
            scan_masked, in_block_comment = _mask_strings_and_comments(lines[scan_index], in_block_comment)
            scan_stripped = scan_masked.strip()

            if _is_blank_or_comment(scan_stripped) or scan_stripped.startswith('#'):
                scan_index += 1
                continue

            if not field_chains and scan_stripped in ('{', '}'):
                break

            assignment = _parse_member_assignment(scan_stripped, variable_name)
            if assignment is None:
                break

            field_chain, rhs = assignment
            if _references_variable(rhs, variable_name):
                break

            field_chains.append(field_chain)
            scan_index += 1

        if field_chains:
            violations.append((declaration_line, type_name, variable_name, field_chains))
            line_index = scan_index
        else:
            line_index += 1

    return violations
