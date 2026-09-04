#!/usr/bin/env python3
"""
Uninitialized Declaration Checks

Detects variables declared without initialization ('Type var;') that are
then assigned member by member on the first executable statement that
follows, ignoring blank lines and comments in between. Direct brace
initialization, designated initializers and copy initialization are not
reported, and any interleaved instruction cancels the detection.
"""

import re
from typing import List, Optional, Tuple

from linter.style_checks import _mask_strings_and_comments, _NON_DECLARATION_KEYWORD_RE

UNINITIALIZED_DECL_MESSAGE = (
    "Variable '{name}' déclarée puis initialisée par assignation membre par membre. "
    "Préférer l'initialisation directe ou un designated initializer (C++20)."
)

_DECLARATION_QUALIFIERS_RE = (
    r'(?:(?:const|constexpr|consteval|constinit|static|inline|extern|mutable|'
    r'volatile|unsigned|signed|short|long|thread_local)\s+)*'
)

_DECLARATION_TYPE_NAME_RE = r'[A-Za-z_]\w*(?:\s*::\s*[A-Za-z_]\w*)*'

_DECLARATION_TEMPLATE_ARGS_RE = r'(?:\s*<(?:[^<>]|<(?:[^<>]|<[^<>]*>)*>)*>\s*)?'

_DECLARATION_POINTER_REFS_RE = r'(?:\s*[*&]\s*)*'

_SIMPLE_UNINITIALIZED_DECL_RE = re.compile(
    rf'^{_DECLARATION_QUALIFIERS_RE}'
    rf'{_DECLARATION_TYPE_NAME_RE}'
    rf'{_DECLARATION_TEMPLATE_ARGS_RE}'
    rf'{_DECLARATION_POINTER_REFS_RE}'
    rf'\s*(?P<name>[A-Za-z_]\w*)$'
)

_MEMBER_ASSIGNMENT_RE = re.compile(r'^(?P<name>[A-Za-z_]\w*)\s*\.\s*[A-Za-z_]\w*\s*=(?!=)')


def format_uninitialized_decl_message(variable_name: str) -> str:
    """
    Render the warning message for a variable initialized member by member.
    """
    return UNINITIALIZED_DECL_MESSAGE.format(name=variable_name)


def _parse_uninitialized_declaration(stripped_masked: str) -> Optional[str]:
    """
    Return the declared variable name when the masked statement is a simple
    declaration without initialization ('Type var;'), or None otherwise.
    Brace/parenthesized/copy initializations, prototypes, multiple
    declarators and multi-statement lines are rejected.
    """
    if not stripped_masked.endswith(';'):
        return None

    if _NON_DECLARATION_KEYWORD_RE.match(stripped_masked):
        return None

    declaration_body = stripped_masked[:-1].strip()
    if not declaration_body:
        return None

    match = _SIMPLE_UNINITIALIZED_DECL_RE.match(declaration_body)
    if match is None:
        return None

    return match.group('name')


def check_uninitialized_declarations(code: str) -> List[Tuple[int, str]]:
    """
    Report declarations without initialization whose first following
    executable statement (blank lines and comments ignored) assigns a member
    of the declared variable, as (declaration_line, variable_name) tuples
    with 1-based line numbers. Any other interleaved instruction cancels the
    detection and the scan continues from that line.
    """
    lines = code.splitlines()
    violations: List[Tuple[int, str]] = []
    in_block_comment = False
    pending_name: Optional[str] = None
    pending_line = 0

    for line_index, raw_line in enumerate(lines):
        masked_line, in_block_comment = _mask_strings_and_comments(raw_line, in_block_comment)
        stripped = masked_line.strip()
        if not stripped or stripped.startswith('#'):
            continue

        if pending_name is None:
            declared_name = _parse_uninitialized_declaration(stripped)
            if declared_name is not None:
                pending_name = declared_name
                pending_line = line_index + 1
            continue

        member_match = _MEMBER_ASSIGNMENT_RE.match(stripped)
        if member_match is not None and member_match.group('name') == pending_name:
            violations.append((pending_line, pending_name))
            pending_name = None
            continue

        declared_name = _parse_uninitialized_declaration(stripped)
        if declared_name is not None:
            pending_name = declared_name
            pending_line = line_index + 1
        else:
            pending_name = None

    return violations