#!/usr/bin/env python3
"""
Advanced Function Brace Formatter

Alternative brace placement pass that also joins multi-line function
signatures onto a single line before putting the brace on its own line.
"""

from typing import List

from shared.brace_utils import (
    find_brace_positions,
    extract_function_name,
)


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
