#!/usr/bin/env python3
"""
Style Checks

Checks for style violations in C++ code: line length, file length and
function length.
"""

import re
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


_NON_DECLARATION_KEYWORD_RE = re.compile(
    r'^(?:return|throw|break|continue|goto|case|default|'
    r'delete|new|sizeof|static_assert|'
    r'using|typedef|namespace|class|struct|enum|union|'
    r'import|module|'
    r'if|for|while|switch|catch|else|do|try)\b'
)

_COMPOUND_ASSIGN_RE = re.compile(r'(?:[-+*/%&|^]=|<<=|>>=)')


def _find_head(line: str) -> str:
    """
    Return the part of the line before the first top-level '=' (standalone,
    not comparison or compound assignment), '{' or ';'. Parenthesis depth is
    tracked so that '=' or '{' inside function calls or nested initializers
    are ignored.
    """
    depth = 0
    i = 0
    length = len(line)
    while i < length:
        char = line[i]
        if char == '(':
            depth += 1
        elif char == ')':
            if depth > 0:
                depth -= 1
        elif depth == 0:
            if char == '=':
                if i + 1 < length and line[i + 1] == '=':
                    i += 2
                    continue
                if i > 0 and line[i - 1] in '!<>':
                    i += 1
                    continue
                return line[:i].strip()
            if char == '{':
                return line[:i].strip()
            if char == ';':
                return line[:i].strip()
        i += 1
    return line.strip()


def _is_declaration_line(masked_stripped: str) -> bool:
    """
    Check whether a code line (strings/comments already masked, whitespace
    stripped) is a local variable declaration or initialisation.
    """
    if _NON_DECLARATION_KEYWORD_RE.match(masked_stripped):
        return False

    if '<<' in masked_stripped or '>>' in masked_stripped:
        return False

    if _COMPOUND_ASSIGN_RE.search(masked_stripped):
        return False

    head = _find_head(masked_stripped)
    tokens = head.split()
    if len(tokens) < 2:
        return False

    first_token = tokens[0]
    if '.' in first_token or '[' in first_token or '(' in first_token:
        return False

    return True


def _scan_body_for_declaration_block(
    lines: List[str],
    brace_line_index: int,
    in_block_comment: bool,
) -> Optional[int]:
    """
    Scan the function body starting after the opening brace at brace_line_index.
    Return the 0-based line index of the last declaration in the first block
    when a blank line is missing after it, or None when the rule is satisfied
    or no declaration block is present.
    """
    body_idx = brace_line_index + 1

    while body_idx < len(lines):
        masked, in_block_comment = _mask_strings_and_comments(lines[body_idx], in_block_comment)
        if masked.strip() == '':
            body_idx += 1
            continue
        break

    if body_idx >= len(lines):
        return None

    first_masked, in_block_comment = _mask_strings_and_comments(lines[body_idx], in_block_comment)
    if not _is_declaration_line(first_masked.strip()):
        return None

    group_end = body_idx
    while group_end < len(lines):
        block_masked, in_block_comment = _mask_strings_and_comments(lines[group_end], in_block_comment)
        block_stripped = block_masked.strip()
        if not block_stripped:
            break
        if not _is_declaration_line(block_stripped):
            break
        while not (block_stripped.endswith(';') or block_stripped.endswith('}')):
            group_end += 1
            if group_end >= len(lines):
                return None
            block_masked, in_block_comment = _mask_strings_and_comments(lines[group_end], in_block_comment)
            block_stripped = block_masked.strip()
        group_end += 1

    if group_end >= len(lines):
        return None

    if lines[group_end].strip() == '':
        return None

    return group_end - 1


def check_blank_line_after_initialization(code: str) -> List[Tuple[str, int, int]]:
    """
    Detect functions where the first block of local variable declarations is not
    separated from the following statements by a blank line.

    Returns a list of (function_name, function_start_line, last_declaration_line)
    tuples (1-based line numbers).
    """
    lines = code.splitlines()
    violations: List[Tuple[str, int, int]] = []
    in_block_comment = False
    paren_depth = 0
    scope_depth = 0

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
                    last_decl = _scan_body_for_declaration_block(
                        lines, line_index, in_block_comment
                    )
                    if last_decl is not None:
                        violations.append((func_name, start_line + 1, last_decl + 1))
            elif char == '}' and paren_depth == 0:
                if scope_depth > 0:
                    scope_depth -= 1

    return violations



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
