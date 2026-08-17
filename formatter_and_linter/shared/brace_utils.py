#!/usr/bin/env python3
"""
Utilities for brace detection and analysis in C++ code.

Provides state-machine helpers for identifying braces in function definitions,
lambdas, initializer lists, and control structures.
"""

import re
from typing import List, Tuple, Optional


CONTROL_KEYWORDS = frozenset({
    'if', 'else', 'for', 'while', 'do', 'switch', 'catch',
    'return', 'throw', 'delete', 'new', 'sizeof', 'alignof',
    'typeid', 'decltype', 'noexcept', 'static_assert'
})


def is_control_structure(line: str) -> bool:
    stripped = line.strip()
    for keyword in ['if', 'while', 'for', 'switch', 'catch']:
        pattern = rf'^{keyword}\s*\('
        if re.match(pattern, stripped):
            return True
    return False


def extract_function_name(line: str) -> Optional[str]:
    stripped = line.strip()
    paren_pos = stripped.find('(')
    if paren_pos == -1:
        return None
    before_paren = stripped[:paren_pos].rstrip()
    if not before_paren:
        return None
    name_end = len(before_paren)
    name_start = name_end - 1
    while name_start >= 0 and (before_paren[name_start].isalnum() or before_paren[name_start] == '_'):
        name_start -= 1
    name_start += 1
    if name_start >= name_end:
        return None
    func_name = before_paren[name_start:name_end]
    if func_name in CONTROL_KEYWORDS:
        return None
    return func_name


def find_brace_positions(line: str) -> List[Tuple[int, str]]:
    positions = []
    in_string = False
    string_char = None
    in_line_comment = False
    in_block_comment = False
    i = 0
    while i < len(line):
        char = line[i]
        next_char = line[i + 1] if i + 1 < len(line) else ''
        if not in_string and not in_line_comment and not in_block_comment:
            if char == '/' and next_char == '/':
                in_line_comment = True
                i += 2
                continue
            elif char == '/' and next_char == '*':
                in_block_comment = True
                i += 2
                continue
            elif char == '"' or char == "'":
                in_string = True
                string_char = char
                i += 1
                continue
            elif char == '{' or char == '}':
                positions.append((i, char))
        elif in_line_comment:
            break
        elif in_block_comment:
            if char == '*' and next_char == '/':
                in_block_comment = False
                i += 2
                continue
        elif in_string:
            if char == '\\':
                i += 2
                continue
            elif char == string_char:
                in_string = False
        i += 1
    return positions


def is_lambda_capture(line: str, brace_pos: int) -> bool:
    before_brace = line[:brace_pos]
    bracket_pattern = r'\[[^\]]*\]\s*$'
    return bool(re.search(bracket_pattern, before_brace))


def is_initializer_list(line: str, brace_pos: int) -> bool:
    before_brace = line[:brace_pos].rstrip()
    if before_brace.endswith('='):
        return True
    type_brace_pattern = r'\w+\s*\{[^}]*\}'
    if re.search(type_brace_pattern, line):
        return True
    return False


def is_constructor_initializer_continuation(line_prefix: str) -> bool:
    stripped = line_prefix.strip()
    return stripped.startswith(':') or stripped.startswith(',')