#!/usr/bin/env python3
"""
Import Order and Spacing

Sorts import statements (system before project) and normalizes blank-line
spacing around module declarations and import blocks.
"""

import re
from typing import List

from shared.regex_patterns import (
    MODULE_DECL_REGEX,
    MODULE_PARTITION_REGEX,
    IMPORT_REGEX,
)
from shared.import_utils import sort_imports


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
