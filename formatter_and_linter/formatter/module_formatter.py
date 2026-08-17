#!/usr/bin/env python3
"""
Module import formatting utilities for C++20 modules.

Handles spacing between module declarations and imports, reordering of using
statements, and import block formatting.
"""

import re
from typing import List, Tuple

from shared.regex_patterns import (
    MODULE_DECL_REGEX,
    MODULE_PARTITION_REGEX,
    IMPORT_REGEX,
    USING_REGEX
)
from shared.import_utils import sort_imports


def is_import_or_include_or_module(line: str) -> bool:
    stripped = line.strip()
    if re.match(r'^\s*#\s*include\s+', line):
        return True
    if re.match(MODULE_DECL_REGEX, line) or re.match(MODULE_PARTITION_REGEX, line):
        return True
    if re.match(IMPORT_REGEX, line):
        return True
    return False


def is_function_prototype(line: str) -> bool:
    stripped = line.strip()
    if not stripped.endswith(';'):
        return False
    if '(' not in stripped or ')' not in stripped:
        return False
    if '{' in stripped:
        return False
    if stripped.startswith('#'):
        return False
    if stripped.startswith('//') or stripped.startswith('/*'):
        return False
    if re.match(r'^\s*typedef\s+', stripped):
        return False
    if re.match(r'^\s*using\s+', stripped):
        return False
    if re.match(r'^\s*(static_assert|noexcept|throw)\s*\(', stripped):
        return False
    if re.search(r'\)\s*=\s*(delete|default)\s*;', stripped):
        return False
    if not re.search(r'\w\s*\(', stripped):
        return False
    return True


def is_function_prototype_start(line: str) -> bool:
    stripped = line.strip()
    if '(' not in stripped:
        return False
    if '{' in stripped:
        return False
    if stripped.startswith('#'):
        return False
    if stripped.startswith('//') or stripped.startswith('/*'):
        return False
    if re.match(r'^\s*typedef\s+', stripped):
        return False
    if re.match(r'^\s*using\s+', stripped):
        return False
    if re.match(r'^\s*(static_assert|noexcept|throw)\s*\(', stripped):
        return False
    if not re.search(r'\w\s*\(', stripped):
        return False
    if stripped.endswith(';'):
        return False
    return True


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


def reorder_using_after_import(code: str) -> str:
    lines = code.splitlines()
    new_lines: List[str] = []
    i = 0

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if re.match(MODULE_DECL_REGEX, line) or re.match(MODULE_PARTITION_REGEX, line):
            new_lines.append(line)
            i += 1
            continue

        if re.match(IMPORT_REGEX, line) or (re.match(USING_REGEX, line) and
            (not new_lines or new_lines[-1].strip() == '' or
             re.match(MODULE_DECL_REGEX, new_lines[-1]) or
             re.match(MODULE_PARTITION_REGEX, new_lines[-1]) or
             re.match(IMPORT_REGEX, new_lines[-1]) or
             re.match(USING_REGEX, new_lines[-1]))):

            imports_list: List[str] = []
            usings: List[str] = []
            trailing_blank_lines: List[str] = []

            while i < len(lines):
                current_line = lines[i]
                current_stripped = current_line.strip()

                if re.match(IMPORT_REGEX, current_line):
                    imports_list.append(current_line)
                    i += 1
                elif re.match(USING_REGEX, current_line):
                    usings.append(current_line)
                    i += 1
                elif current_stripped == '':
                    trailing_blank_lines.append(current_line)
                    i += 1
                else:
                    break

            new_lines.extend(imports_list)
            new_lines.extend(usings)

            if i < len(lines) and lines[i].strip() != '':
                if trailing_blank_lines or (new_lines and new_lines[-1].strip() != ''):
                    new_lines.append('')

            continue

        new_lines.append(line)
        i += 1

    return '\n'.join(new_lines)


def format_import_order(code: str) -> str:
    lines = code.splitlines()
    new_lines: List[str] = []
    i = 0

    while i < len(lines):
        line = lines[i]

        if re.match(IMPORT_REGEX, line):
            imports = []
            while i < len(lines):
                current_line = lines[i]
                current_stripped = current_line.strip()
                if re.match(IMPORT_REGEX, current_line):
                    imports.append(current_line)
                    i += 1
                elif current_stripped == '':
                    i += 1
                else:
                    break

            system_imports, local_imports = sort_imports(imports)

            formatted_imports: List[str] = []
            formatted_imports.extend(system_imports)
            if system_imports and local_imports:
                formatted_imports.append('')
            formatted_imports.extend(local_imports)

            new_lines.extend(formatted_imports)
            continue

        new_lines.append(line)
        i += 1

    return '\n'.join(new_lines)


def format_module_import_spacing(code: str) -> str:
    lines = code.splitlines()
    new_lines: List[str] = []
    i = 0

    while i < len(lines):
        line = lines[i]
        stripped = line.strip()

        if re.match(MODULE_DECL_REGEX, line) or re.match(MODULE_PARTITION_REGEX, line):
            new_lines.append(line)
            i += 1

            while i < len(lines) and lines[i].strip() == '':
                i += 1

            if i < len(lines) and lines[i].strip() != '':
                new_lines.append('')

            continue

        if re.match(IMPORT_REGEX, line):
            imports = []
            while i < len(lines) and re.match(IMPORT_REGEX, lines[i]):
                imports.append(lines[i])
                i += 1

            need_blank_before = False
            if new_lines:
                last_line = new_lines[-1]
                if not re.match(MODULE_DECL_REGEX, last_line) and \
                   not re.match(MODULE_PARTITION_REGEX, last_line):
                    if last_line.strip() != '':
                        need_blank_before = True

            if need_blank_before:
                new_lines.append('')

            new_lines.extend(imports)

            while i < len(lines) and lines[i].strip() == '':
                i += 1

            if i < len(lines) and lines[i].strip() != '':
                next_line = lines[i]
                if not re.match(IMPORT_REGEX, next_line) and \
                   not re.match(MODULE_DECL_REGEX, next_line) and \
                   not re.match(MODULE_PARTITION_REGEX, next_line):
                    new_lines.append('')

            continue

        new_lines.append(line)
        i += 1

    return '\n'.join(new_lines)