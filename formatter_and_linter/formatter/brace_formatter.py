#!/usr/bin/env python3
"""
Brace Formatter

Ensures opening curly braces of function definitions are placed on a new line,
while opening braces of control structures (if, else, for, while, switch,
catch, do, try) are placed on the same line as their header.
"""

import re
from typing import List, Optional

from shared.brace_utils import (
    find_brace_positions,
    is_lambda_capture,
    is_initializer_list,
    extract_function_name,
    is_constructor_initializer_continuation,
)
from shared.function_analysis import (
    find_function_start_for_brace,
)


CONTROL_HEADER_PATTERN = re.compile(r'^\s*(?:if|else|for|while|switch|catch|do|try)\b')


def format_control_structure_braces(code: str) -> str:
    """
    Place the opening brace of control structures (if/else/for/while/switch/
    catch/do/try) on the same line as their header.

    Function definitions, classes, structs, namespaces and enums are left
    untouched (their brace stays on its own line).
    """
    lines = code.splitlines()
    emit = [True] * len(lines)
    total = len(lines)
    i = 0

    while i < total:
        if lines[i].strip() == '{':
            start_idx = _find_statement_start_backward(lines, i)

            if start_idx is not None and CONTROL_HEADER_PATTERN.match(lines[start_idx]):
                target_idx = _last_content_line_before(lines, i)

                if target_idx is not None and target_idx >= start_idx:
                    # Join the brace onto the last header line, keeping any
                    # trailing line-comment after the brace
                    code_part, comment = _split_trailing_comment(lines[target_idx])
                    if comment is not None:
                        lines[target_idx] = code_part + ' { ' + comment
                    else:
                        lines[target_idx] = code_part + ' {'

                    # Drop the now merged standalone '{' line
                    emit[i] = False
                    i += 1
                    continue

        i += 1

    return '\n'.join(line for keep, line in zip(emit, lines) if keep)


def _find_statement_start_backward(lines: List[str], brace_idx: int) -> Optional[int]:
    """
    Walk backward from the standalone '{' at brace_idx to find the first line
    of the statement owning that brace. Returns the line index, or None when
    no consistent statement start can be found.
    """
    balance = 0
    j = brace_idx - 1

    while j >= 0:
        stripped = lines[j].strip()

        if not stripped or stripped.startswith('//'):
            j -= 1
            continue

        balance += stripped.count('(') - stripped.count(')')

        if balance >= 0:
            return j

        j -= 1

    return None


def _last_content_line_before(lines: List[str], brace_idx: int) -> Optional[int]:
    """Return the index of the last non-blank, non-comment line before brace_idx."""
    j = brace_idx - 1

    while j >= 0:
        stripped = lines[j].strip()

        if stripped and not stripped.startswith('//'):
            return j

        j -= 1

    return None


def _split_trailing_comment(line: str) -> tuple[str, Optional[str]]:
    """
    Split a line into (code, comment) where comment is a trailing '//' comment
    (string literals are respected), or None when the line has no comment.
    """
    in_string = False
    string_char = None
    i = 0

    while i < len(line):
        char = line[i]

        if in_string:
            if char == '\\':
                i += 2
                continue
            if char == string_char:
                in_string = False
        elif char == '"' or char == "'":
            in_string = True
            string_char = char
        elif char == '/' and i + 1 < len(line) and line[i + 1] == '/':
            return line[:i].rstrip(), line[i:]

        i += 1

    return line.rstrip(), None


def format_function_braces(code: str) -> str:
    lines = code.splitlines()
    result_lines: List[str] = []
    brace_depth = 0
    paren_depth = 0
    angle_depth = 0

    for line_idx, line in enumerate(lines):
        brace_positions = find_brace_positions(line)

        if not brace_positions:
            result_lines.append(line)
            paren_depth += line.count('(') - line.count(')')
            angle_depth = _update_angle_depth(line, angle_depth)
            continue

        processed = False

        for pos, brace_type in brace_positions:
            if brace_type == '{':
                before_brace = line[:pos]

                if is_lambda_capture(line, pos):
                    result_lines.append(line)
                    processed = True
                    break

                if is_initializer_list(line, pos):
                    result_lines.append(line)
                    processed = True
                    break

                local_paren_depth = before_brace.count('(') - before_brace.count(')')

                if local_paren_depth == 0 and '(' in before_brace:
                    func_name = extract_function_name(before_brace)

                    if func_name is not None:
                        after_brace = line[pos + 1:]
                        func_decl = before_brace.rstrip()
                        base_indent = len(func_decl) - len(func_decl.lstrip())

                        if is_constructor_initializer_continuation(before_brace):
                            func_start_idx = find_function_start_for_brace(lines, line_idx + 1)
                            if func_start_idx is not None:
                                func_start_line = lines[func_start_idx]
                                base_indent = len(func_start_line) - len(func_start_line.lstrip())

                        brace_indent = ' ' * base_indent

                        result_lines.append(func_decl)
                        result_lines.append(brace_indent + '{')

                        if after_brace.strip():
                            content = after_brace.strip()
                            if content.endswith('}'):
                                body_content = content[:-1].rstrip().rstrip(';')
                                if body_content:
                                    result_lines.append(brace_indent + '    ' + body_content + ';')
                                result_lines.append(brace_indent + '}')
                            else:
                                result_lines.append(brace_indent + '    ' + content)

                        processed = True
                        brace_depth += 1
                        break

        if not processed and line.strip() == '{':
            func_start_idx = find_function_start_for_brace(result_lines, len(result_lines))
            if func_start_idx is not None:
                func_line = result_lines[func_start_idx] if func_start_idx < len(result_lines) else ""
                base_indent = len(func_line) - len(func_line.lstrip())
                result_lines.append(' ' * base_indent + '{')
                processed = True

        if not processed:
            result_lines.append(line)
            for _, btype in brace_positions:
                if btype == '{':
                    brace_depth += 1
                else:
                    brace_depth -= 1

        paren_depth += line.count('(') - line.count(')')
        angle_depth = _update_angle_depth(line, angle_depth)

    return '\n'.join(result_lines)


def _update_angle_depth(line: str, angle_depth: int) -> int:
    for char in line:
        if char == '<':
            if angle_depth > 0 or (line.strip() and line.strip()[-1:].isalnum()):
                angle_depth += 1
        elif char == '>':
            if angle_depth > 0:
                angle_depth -= 1
    return angle_depth


def format_function_braces_advanced(code: str) -> str:
    lines = code.splitlines()
    result_lines: List[str] = []
    i = 0

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if '(' in stripped and not stripped.endswith('{') and not stripped.endswith(';'):
            func_name = extract_function_name(stripped)
            if func_name is not None and not stripped.startswith(('if', 'while', 'for', 'switch', 'catch')):
                paren_depth = stripped.count('(') - stripped.count(')')
                j = i + 1

                while j < len(lines) and paren_depth > 0:
                    next_line = lines[j].strip()
                    paren_depth += next_line.count('(') - next_line.count(')')
                    j += 1

                if paren_depth == 0 and j < len(lines):
                    next_stripped = lines[j].strip()
                    if next_stripped.startswith('{'):
                        func_lines = lines[i:j]
                        func_decl = ' '.join(l.strip() for l in func_lines)
                        result_lines.append(func_decl.rstrip())
                        result_lines.append('{')
                        after_brace = lines[j].strip()[1:].lstrip()
                        if after_brace:
                            result_lines.append(after_brace)
                        i = j + 1
                        continue

        brace_positions = find_brace_positions(line)
        if brace_positions:
            for pos, brace_type in brace_positions:
                if brace_type == '{':
                    before_brace = line[:pos]
                    local_paren_depth = before_brace.count('(') - before_brace.count(')')
                    if local_paren_depth == 0 and '(' in before_brace:
                        func_name = extract_function_name(before_brace)
                        if func_name is not None:
                            after_brace = line[pos + 1:].lstrip()
                            func_decl = before_brace.rstrip()
                            result_lines.append(func_decl)
                            result_lines.append('{')
                            if after_brace:
                                result_lines.append(after_brace)
                            break
            else:
                result_lines.append(line)
        else:
            result_lines.append(line)
        i += 1

    return '\n'.join(result_lines)