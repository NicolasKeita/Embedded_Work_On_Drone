#!/usr/bin/env python3
"""
Utilities for function context analysis in C++ code.

Provides functions for detecting function definition context, finding function
starts, and checking multiline function signatures.
"""

from typing import List, Optional

from shared.brace_utils import (
    CONTROL_KEYWORDS,
    extract_function_name,
    is_control_structure,
)


def is_function_definition_context(lines: List[str], line_idx: int, brace_line: str, paren_depth: int, angle_depth: int) -> bool:
    if paren_depth != 0 or angle_depth != 0:
        return False
    if is_control_structure(brace_line):
        return False
    func_name = extract_function_name(brace_line)
    if func_name is None:
        return check_multiline_function_signature(lines, line_idx)
    return True


def check_multiline_function_signature(lines: List[str], line_idx: int) -> bool:
    paren_depth = 0
    for i in range(line_idx - 1, -1, -1):
        line = lines[i].strip()
        if not line or line.startswith('//'):
            continue
        if line == '{':
            continue
        if line.startswith(':') or line.startswith(','):
            continue
        for char in reversed(line):
            if char == ')':
                paren_depth += 1
            elif char == '(':
                paren_depth -= 1
                if paren_depth < 0:
                    func_name = extract_function_name(line)
                    if func_name is not None:
                        return True
                    if is_control_structure(line):
                        return False
                    return False
        if line.endswith(';') or (line.endswith('}') and line != '{'):
            return False
    return False


def find_function_start_for_brace(lines: List[str], line_idx: int) -> Optional[int]:
    for i in range(line_idx - 1, -1, -1):
        line = lines[i].strip()
        if not line or line.startswith('//'):
            continue
        if line == '{' or line == '}':
            continue
        if line.startswith(':'):
            continue
        if line.startswith(','):
            continue
        if line.endswith(')'):
            paren_depth = 1
            for char in reversed(line[:-1]):
                if char == ')':
                    paren_depth += 1
                elif char == '(':
                    paren_depth -= 1
                    if paren_depth == 0:
                        func_name = extract_function_name(line)
                        if func_name is not None:
                            return i
                        return None
            for j in range(i - 1, -1, -1):
                check_line = lines[j]
                for char in reversed(check_line):
                    if char == ')':
                        paren_depth += 1
                    elif char == '(':
                        paren_depth -= 1
                        if paren_depth == 0:
                            func_name = extract_function_name(check_line)
                            if func_name is not None:
                                return j
                            return None
            return None
    return None