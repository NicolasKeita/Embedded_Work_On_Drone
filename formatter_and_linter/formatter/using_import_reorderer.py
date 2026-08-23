#!/usr/bin/env python3
"""
Using Statement Reordering After Imports

Groups top-level using statements right after the import block that follows
the module declaration.
"""

import re
from typing import List

from shared.regex_patterns import (
    MODULE_DECL_REGEX,
    MODULE_PARTITION_REGEX,
    IMPORT_REGEX,
    USING_REGEX,
)


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
