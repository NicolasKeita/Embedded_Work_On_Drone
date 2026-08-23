#!/usr/bin/env python3
"""
Using Statement Reordering After Prototypes

Moves top-level using statements after the last top-level import/include/
module declaration or function prototype.
"""

import re
from typing import List, Tuple

from shared.regex_patterns import USING_REGEX

from formatter.prototype_detection import (
    is_import_or_include_or_module,
    is_function_prototype,
    is_function_prototype_start,
)


def reorder_using_after_prototypes(code: str) -> str:
    lines = code.splitlines()
    brace_depths: List[int] = []
    current_depth = 0
    for line in lines:
        brace_depths.append(current_depth)
        for char in line:
            if char == '{':
                current_depth += 1
            elif char == '}':
                current_depth -= 1
                if current_depth < 0:
                    current_depth = 0

    top_level_usings: List[Tuple[int, str]] = []
    top_level_prototype_end_lines: List[int] = []
    last_import_include_module_pos: int = -1

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if brace_depths[i] == 0:
            if is_import_or_include_or_module(line):
                last_import_include_module_pos = i
                i += 1
                continue

            if re.match(USING_REGEX, line):
                top_level_usings.append((i, line))
                i += 1
                continue

            if is_function_prototype(line):
                top_level_prototype_end_lines.append(i)
                i += 1
                continue

            if is_function_prototype_start(line):
                prototype_end = i
                j = i + 1
                found_semicolon = False
                found_brace = False
                while j < len(lines) and brace_depths[j] == 0:
                    next_line = lines[j]
                    next_stripped = next_line.strip()
                    if next_stripped.endswith(';') and '{' not in next_stripped:
                        prototype_end = j
                        found_semicolon = True
                        break
                    elif next_stripped.endswith('{') or '{' in next_stripped:
                        found_brace = True
                        break
                    j += 1

                if found_semicolon:
                    top_level_prototype_end_lines.append(prototype_end)
                    i = prototype_end + 1
                    continue
                elif found_brace:
                    pass

        i += 1

    if not top_level_prototype_end_lines or not top_level_usings:
        return code

    last_prototype_pos = max(top_level_prototype_end_lines)

    if last_import_include_module_pos > last_prototype_pos:
        insertion_pos = last_import_include_module_pos
    else:
        insertion_pos = last_prototype_pos

    using_indices = {pos for pos, _ in top_level_usings}
    new_lines: List[str] = []
    usings_inserted = False

    for i, line in enumerate(lines):
        if i in using_indices:
            continue
        new_lines.append(line)
        if i == insertion_pos and not usings_inserted:
            if top_level_usings:
                new_lines.append('')
                for _, using_line in top_level_usings:
                    new_lines.append(using_line)
            usings_inserted = True

    return '\n'.join(new_lines)
