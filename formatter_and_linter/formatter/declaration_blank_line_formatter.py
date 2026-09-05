#!/usr/bin/env python3
"""
Declaration Blank-Line Formatter

Removes the blank lines that separate consecutive local variable declarations at
the very beginning of a C++ function body, packing the declaration block together.

Rules implemented
-----------------
* The "declaration zone" starts immediately after the opening brace ``{`` of a
  function definition, whether that brace sits on the signature line
  (``void f() {``) or on its own line (``void f()\\n{``).
* Inside the zone, every blank line located *between* two consecutive
  declaration statements is removed.
* The declaration forms recognised are the ones covered by the shared
  ``DECLARATION_PATTERN`` (basic declarations, assignments, uniform/aggregate
  initialisers and templates with a constructor call).
* The first line that is not a declaration (control structure, function call,
  reassignment, ...) ends the zone and is left untouched.  Blank lines that
  follow the last declaration -- the separator between the declaration block
  and the rest of the body -- are preserved as-is.

This formatter is the complement of ``format_initialization_blocks``: the
latter guarantees one blank line *after* the declaration block, this one
removes the blank lines *between* the declarations.
"""

import re
from typing import List

from shared.brace_utils import (
    brace_delta,
    find_brace_positions,
    is_lambda_capture,
    is_initializer_list,
    extract_function_name,
)
from shared.function_analysis import find_function_start_for_brace
from formatter.initialization_block_formatter import DECLARATION_PATTERN


_CONTROL_BLOCK_RE = re.compile(r'\b(class|struct|enum|namespace|do|else|try)\b')


def _prev_nonempty(lines: List[str], idx: int) -> str:
    j = idx - 1
    while j >= 0:
        stripped = lines[j].strip()
        if stripped:
            return stripped
        j -= 1
    return ''


def is_function_open_brace(line: str, lines: List[str], idx: int) -> bool:
    """
    Return True when ``line`` carries the opening brace of a function
    definition (same-line brace or a lone ``{`` preceded by a signature).
    """
    positions = find_brace_positions(line)
    if not any(char == '{' for _, char in positions):
        return False

    for pos, char in positions:
        if char != '{':
            continue
        before = line[:pos]
        if is_lambda_capture(line, pos):
            continue
        if is_initializer_list(line, pos):
            continue
        local_paren_depth = before.count('(') - before.count(')')
        if local_paren_depth == 0 and '(' in before:
            if extract_function_name(before) is not None:
                return True

    stripped = line.strip()
    if stripped == '{':
        previous = _prev_nonempty(lines, idx)
        if not previous or _CONTROL_BLOCK_RE.search(previous):
            return False
        return find_function_start_for_brace(lines, idx) is not None

    return False


def _statement_end(stripped: str, depth: int) -> bool:
    return depth <= 1 and (stripped.endswith(';') or stripped.endswith('}'))


def _process_declaration_zone(lines: List[str], start: int, result: List[str]) -> int:
    """
    Walk the leading declaration zone of a function body starting at ``start``,
    appending to ``result`` and returning the index at which normal scanning
    should resume.
    """
    n = len(lines)
    i = start
    brace_depth = 1
    pending: List[str] = []
    has_seen_declaration = False

    while i < n:
        line = lines[i]
        stripped = line.strip()
        delta = brace_delta(line)

        if brace_depth + delta <= 0:
            result.extend(pending)
            result.append(line)
            return i + 1

        if not stripped:
            pending.append(line)
            i += 1
            continue

        if stripped.startswith('//'):
            result.extend(pending)
            result.append(line)
            return i + 1

        if stripped.startswith('/*'):
            result.extend(pending)
            result.append(line)
            j = i
            while j + 1 < n and '*/' not in lines[j]:
                j += 1
                result.append(lines[j])
            return j + 1

        if DECLARATION_PATTERN.match(stripped):
            if has_seen_declaration:
                pending = []
            else:
                result.extend(pending)
                pending = []
            has_seen_declaration = True
            result.append(line)
            brace_depth += delta
            i += 1
            if not _statement_end(stripped, brace_depth):
                while i < n:
                    cline = lines[i]
                    cstrip = cline.strip()
                    cdelta = brace_delta(cline)
                    result.append(cline)
                    brace_depth += cdelta
                    i += 1
                    if _statement_end(cstrip, brace_depth) or brace_depth <= 0:
                        break
            continue

        result.extend(pending)
        result.append(line)
        return i + 1

    return i


def remove_blank_lines_between_declarations(code: str) -> str:
    """
    Remove blank lines located between consecutive variable declarations at the
    beginning of every C++ function body found in ``code``.
    """
    lines = code.splitlines()
    result: List[str] = []
    i = 0
    n = len(lines)

    while i < n:
        line = lines[i]
        if is_function_open_brace(line, lines, i):
            if brace_delta(line) <= 0:
                result.append(line)
                i += 1
                continue
            result.append(line)
            i = _process_declaration_zone(lines, i + 1, result)
            continue
        result.append(line)
        i += 1

    return '\n'.join(result) + ('\n' if code.endswith('\n') else '')


__all__ = ["remove_blank_lines_between_declarations", "is_function_open_brace"]
