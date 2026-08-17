#!/usr/bin/env python3
"""
Comment-function spacing formatter for the C++ code formatter.

Handles proper spacing rules where comments above functions have no blank line,
while other code elements maintain proper separation.
"""

import re
from typing import List

from shared.regex_patterns import FUNC_REGEX


def format_comment_function_spacing(code: str) -> str:
    lines = code.splitlines()
    new_lines = []
    prev_blank = False

    for i, line in enumerate(lines):
        stripped = line.strip()

        if stripped == '':
            if not prev_blank:
                new_lines.append('')
                prev_blank = True
            continue

        if re.match(FUNC_REGEX, line):
            j = len(new_lines) - 1
            while j >= 0 and new_lines[j].strip() == '':
                j -= 1

            if j >= 0:
                last_line = new_lines[j].strip()
            else:
                last_line = None

            if last_line is not None and (last_line.startswith('//') or last_line.startswith('/*')):
                while new_lines and new_lines[-1].strip() == '':
                    new_lines.pop()
            else:
                if not new_lines or new_lines[-1].strip() != '':
                    if last_line is not None and re.match(FUNC_REGEX, new_lines[j]):
                        pass
                    else:
                        new_lines.append('')

            new_lines.append(line)
            prev_blank = False
        else:
            new_lines.append(line)
            prev_blank = False

    return '\n'.join(new_lines)