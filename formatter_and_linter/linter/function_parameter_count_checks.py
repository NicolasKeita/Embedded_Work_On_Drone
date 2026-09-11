#!/usr/bin/env python3
"""
Function Parameter Count Checks

Warns when a function definition has more than MAX_FUNCTION_PARAMETERS
parameters (five is allowed, six or more is reported). The rule applies
to both .cpp and .cppm files.

The check reuses the sanitizer and block-classification logic from
cppm_inline_function_checks to identify function definitions (an opening
brace at parenthesis depth zero whose preceding header is classified as a
function). For each function the parameter list is extracted and the
top-level commas are counted; commas nested inside parentheses, angle
brackets, square brackets and braces are ignored so that function-pointer
parameters, template arguments and braced default values are handled
correctly.
"""

from typing import List, Tuple

from linter.cppm_inline_function_checks import (
    _Block,
    _BlockKind,
    _classify_header,
    _compute_line_starts,
    _find_parameter_list_paren,
    _is_braced_initializer,
    _line_of_position,
    _sanitize_code,
    _split_header,
)

MAX_FUNCTION_PARAMETERS = 5

WARN_FUNCTION_TOO_MANY_PARAMETERS_TAG = "[WARN_FUNCTION_TOO_MANY_PARAMETERS]"


def _count_parameters(header: str) -> int:
    """
    Return the number of parameters declared in a function signature header.

    The header is normalised (whitespace collapsed) before the parameter
    list is extracted from the first top-level parenthesis pair. Empty
    lists '()' and the C-style '(void)' yield zero. Top-level commas are
    counted; commas nested inside parentheses, angle brackets, square
    brackets and braces are ignored.
    """
    normalized = ' '.join(header.split())
    paren_index = _find_parameter_list_paren(normalized)
    if paren_index == -1:
        return 0

    depth = 0
    close_index = -1
    for i in range(paren_index, len(normalized)):
        char = normalized[i]
        if char == '(':
            depth += 1
        elif char == ')':
            depth -= 1
            if depth == 0:
                close_index = i
                break
    if close_index == -1:
        return 0

    param_list = normalized[paren_index + 1:close_index].strip()
    if not param_list:
        return 0
    if param_list == 'void':
        return 0

    count = 1
    paren_depth = 0
    angle_depth = 0
    bracket_depth = 0
    brace_depth = 0
    for char in param_list:
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
        elif char == '[':
            bracket_depth += 1
        elif char == ']':
            if bracket_depth > 0:
                bracket_depth -= 1
        elif char == '{':
            brace_depth += 1
        elif char == '}':
            if brace_depth > 0:
                brace_depth -= 1
        elif (char == ','
              and paren_depth == 0
              and angle_depth == 0
              and bracket_depth == 0
              and brace_depth == 0):
            count += 1
    return count


def _scan_for_parameter_count(
    sanitized: str,
    max_params: int,
) -> List[Tuple[str, int, int]]:
    """
    Scan sanitised code for function definitions whose parameter count
    exceeds max_params. Returns a list of (name, start_line, param_count)
    tuples sorted by line number.
    """
    line_starts = _compute_line_starts(sanitized)
    blocks: List[_Block] = []
    violations: List[Tuple[str, int, int]] = []
    segment_start = 0
    paren_depth = 0
    index = 0
    length = len(sanitized)

    while index < length:
        char = sanitized[index]
        if char == '(':
            paren_depth += 1
        elif char == ')':
            paren_depth = max(0, paren_depth - 1)
        elif char == '{' and paren_depth == 0:
            if _is_braced_initializer(sanitized, segment_start, index):
                blocks.append(_Block(_BlockKind.OTHER, '', index, index, True))
            else:
                header = sanitized[segment_start:index]
                significant_header, offset = _split_header(header)
                kind, name = _classify_header(significant_header)
                blocks.append(
                    _Block(kind, name, index, segment_start + offset, False)
                )
                if kind == _BlockKind.FUNCTION:
                    param_count = _count_parameters(significant_header)
                    if param_count > max_params:
                        start_line = _line_of_position(
                            line_starts, segment_start + offset
                        ) + 1
                        violations.append((name, start_line, param_count))
                segment_start = index + 1
        elif char == '}' and paren_depth == 0:
            if blocks:
                block = blocks.pop()
                if not block.transparent:
                    segment_start = index + 1
            else:
                segment_start = index + 1
        elif char == ';' and paren_depth == 0:
            segment_start = index + 1
        index += 1

    violations.sort(key=lambda violation: violation[1])
    return violations


def check_function_parameter_count(
    code: str,
    max_params: int = MAX_FUNCTION_PARAMETERS,
) -> List[Tuple[str, int, int]]:
    """
    Report functions whose parameter count exceeds max_params.

    Returns a list of (function_name, start_line, parameter_count) tuples.
    Only function definitions (with a body) are checked; declarations and
    function calls are not affected.
    """
    sanitized = _sanitize_code(code)
    return _scan_for_parameter_count(sanitized, max_params)


def format_function_parameter_count_message(
    file_path: str,
    function_name: str,
    start_line: int,
    param_count: int,
) -> str:
    return (
        f"{WARN_FUNCTION_TOO_MANY_PARAMETERS_TAG} {file_path}:{start_line} : "
        f"la fonction '{function_name}' possède {param_count} paramètres "
        f"(max : {MAX_FUNCTION_PARAMETERS}). "
        "Envisagez de regrouper certains paramètres dans une structure."
    )

