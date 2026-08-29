#!/usr/bin/env python3
"""
Style Checks

Checks for style violations in C++ code: line length, file length and
function length.
"""

from typing import List, Optional, Tuple

from shared.brace_utils import extract_function_name
from shared.function_analysis import strip_trailing_qualifiers

MAX_FUNCTION_LENGTH = 40
MAX_FILE_LENGTH = 120
MAX_SIGNATURE_SCAN_LINES = 50


def check_line_length(code: str, max_length: int = 120) -> List[Tuple[int, int]]:
    lines = code.splitlines()
    long_lines = []
    for i, line in enumerate(lines, 1):
        if len(line) > max_length:
            long_lines.append((i, len(line)))
    return long_lines


def check_file_length(code: str, max_lines: int = MAX_FILE_LENGTH) -> Tuple[bool, int]:
    line_count = len(code.splitlines())
    return (line_count > max_lines, line_count)


def _mask_strings_and_comments(line: str, in_block_comment: bool) -> Tuple[str, bool]:
    """
    Replace string literals, character literals and comments with spaces so
    brace and parenthesis tracking only sees code characters. Returns the
    masked line and the updated block-comment state for multi-line comments.
    """
    masked: List[str] = []
    i = 0
    length = len(line)

    while i < length:
        char = line[i]

        if in_block_comment:
            if char == '*' and i + 1 < length and line[i + 1] == '/':
                in_block_comment = False
                masked.append('  ')
                i += 2
                continue
            masked.append(' ')
            i += 1
            continue

        if char == '/' and i + 1 < length and line[i + 1] == '/':
            masked.append(' ' * (length - i))
            break

        if char == '/' and i + 1 < length and line[i + 1] == '*':
            in_block_comment = True
            masked.append('  ')
            i += 2
            continue

        if char == '"' or char == "'":
            quote = char
            masked.append(' ')
            i += 1
            while i < length:
                if line[i] == '\\':
                    masked.append('  ')
                    i += 2
                    continue
                masked.append(' ')
                i += 1
                if line[i - 1] == quote:
                    break
            continue

        masked.append(char)
        i += 1

    return ''.join(masked), in_block_comment


def _find_matching_paren(text: str) -> Optional[int]:
    """
    Return the index of the opening parenthesis matching the closing
    parenthesis at the end of the text, or None when unbalanced.
    """
    depth = 0
    for i in range(len(text) - 1, -1, -1):
        char = text[i]
        if char == ')':
            depth += 1
        elif char == '(':
            depth -= 1
            if depth == 0:
                return i
    return None


def _classify_signature(signature_parts: List[str], start_line: Optional[int], line_idx: int) -> Tuple[str, Optional[Tuple[str, int]]]:
    """
    Classify the accumulated signature text as 'found' (function definition,
    result carries the name and start line), 'invalid' (balanced parentheses
    but no function name, e.g. a control structure) or 'incomplete' (the
    parameter list is not balanced yet and more lines must be accumulated).
    """
    signature = ' '.join(signature_parts)
    if not signature:
        return ('incomplete', None)
    effective_signature = strip_trailing_qualifiers(signature)
    if not effective_signature.endswith(')'):
        return ('incomplete', None)
    paren_index = _find_matching_paren(effective_signature)
    if paren_index is None:
        return ('incomplete', None)
    func_name = extract_function_name(effective_signature[:paren_index] + '(')
    if func_name is None:
        return ('invalid', None)
    return ('found', (func_name, start_line if start_line is not None else line_idx))


def _find_function_opening(lines: List[str], line_idx: int, head: str) -> Optional[Tuple[str, int]]:
    """
    Determine whether the opening brace at line_idx belongs to a function
    definition, scanning backwards across the (possibly multi-line) signature.
    The text before the brace on the same line seeds the scan; previous lines
    are accumulated until the parameter list balances. Returns the function
    name and the signature start line, or None when the brace does not open a
    function body (control structure, namespace, class, scope block, ...).
    """
    signature_parts: List[str] = []
    start_line: Optional[int] = None

    stripped_head = head.strip()
    if stripped_head:
        signature_parts.append(strip_trailing_qualifiers(stripped_head))
        start_line = line_idx
        status, result = _classify_signature(signature_parts, start_line, line_idx)
        if status == 'found':
            return result
        if status == 'invalid':
            return None

    scan_idx = line_idx - 1
    while scan_idx >= 0 and len(signature_parts) < MAX_SIGNATURE_SCAN_LINES:
        masked_line, _ = _mask_strings_and_comments(lines[scan_idx], False)
        stripped_line = masked_line.strip()
        if (not stripped_line
                or stripped_line.startswith('#')
                or stripped_line.startswith(':')
                or stripped_line.startswith(',')):
            scan_idx -= 1
            continue
        if stripped_line == '{' or stripped_line == '}' or stripped_line.endswith(';'):
            return None

        signature_parts.insert(0, stripped_line)
        start_line = scan_idx
        scan_idx -= 1

        status, result = _classify_signature(signature_parts, start_line, line_idx)
        if status == 'found':
            return result
        if status == 'invalid':
            return None

    return None


def check_function_length(code: str, max_lines: int = MAX_FUNCTION_LENGTH) -> List[Tuple[str, int, int]]:
    """
    Report functions longer than max_lines as (name, start_line, line_count)
    tuples. Function signatures spanning several lines are supported: each
    opening brace at parenthesis depth zero is classified as a function body
    or another scope (control structure, namespace, class, block), and the
    function length is measured from its signature start line to its closing
    brace line.
    """
    lines = code.splitlines()
    long_functions: List[Tuple[str, int, int]] = []
    in_block_comment = False
    paren_depth = 0
    scope_depth = 0
    open_functions: List[Tuple[int, str, int]] = []

    for line_index, raw_line in enumerate(lines):
        masked_line, in_block_comment = _mask_strings_and_comments(raw_line, in_block_comment)
        if masked_line.strip().startswith('#'):
            continue

        for char_index, char in enumerate(masked_line):
            if char == '(':
                paren_depth += 1
            elif char == ')':
                if paren_depth > 0:
                    paren_depth -= 1
            elif char == '{' and paren_depth == 0:
                opening = _find_function_opening(lines, line_index, masked_line[:char_index])
                scope_depth += 1
                if opening is not None:
                    func_name, start_line = opening
                    open_functions.append((scope_depth - 1, func_name, start_line))
            elif char == '}' and paren_depth == 0:
                if scope_depth > 0:
                    scope_depth -= 1
                while open_functions and open_functions[-1][0] == scope_depth:
                    _, func_name, start_line = open_functions.pop()
                    function_lines = line_index - start_line + 1
                    if function_lines > max_lines:
                        long_functions.append((func_name, start_line + 1, function_lines))

    return long_functions
