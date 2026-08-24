#!/usr/bin/env python3
"""
Style Checks

Checks for style violations in C++ code: line length, file length and
function length.
"""

import re
from typing import List, Tuple

MAX_FUNCTION_LENGTH = 35
MAX_FILE_LENGTH = 120


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


def check_function_length(code: str, max_lines: int = MAX_FUNCTION_LENGTH) -> List[Tuple[str, int, int]]:
    lines = code.splitlines()
    long_functions = []
    brace_stack = []
    current_depth = 0

    function_pattern = re.compile(
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+)*'
        r'[\w:]+\s*[*&]?\s+'
        r'(\w+(?:::\w+)?)\s*'
        r'\([^)]*\)\s*'
        r'(?:const\s*)?(?:final\s*|override\s*)?(?:\{|$)'
    )

    pending_function = None

    for i, line in enumerate(lines, 1):
        match = function_pattern.search(line)
        if match:
            func_name = match.group(1)
            if '{' in line:
                brace_stack.append((current_depth, func_name, i))
                current_depth += 1
            else:
                pending_function = (func_name, i)

        if pending_function and line.strip() == '{':
            func_name, line_num = pending_function
            brace_stack.append((current_depth, func_name, line_num))
            current_depth += 1
            pending_function = None
            continue

        open_braces = line.count('{')
        close_braces = line.count('}')

        braces_to_count = open_braces
        if match and '{' in line:
            braces_to_count -= 1

        for _ in range(braces_to_count):
            current_depth += 1

        if close_braces > 0:
            for _ in range(close_braces):
                current_depth -= 1
                if brace_stack and current_depth == brace_stack[-1][0]:
                    _, func_name, start_line = brace_stack.pop()
                    func_lines = i - start_line + 1
                    if func_lines > max_lines:
                        long_functions.append((func_name, start_line, func_lines))

    return long_functions
