#!/usr/bin/env python3
"""
Spacing utilities for the C++ code formatter.

Manages blank line separation between functions and other code elements.
"""

from typing import List

from shared.regex_patterns import FUNC_REGEX
import re


def ensure_single_blank_lines(lines: List[str]) -> List[str]:
    new_lines = []
    prev_blank = False

    for line in lines:
        stripped = line.strip()

        if re.match(FUNC_REGEX, line):
            if new_lines:
                last_line = new_lines[-1].strip()
                if last_line != '' and not last_line.startswith('//') and not last_line.startswith('/*'):
                    if re.match(FUNC_REGEX, new_lines[-1]):
                        pass
                    else:
                        new_lines.append('')
            new_lines.append(line)
            prev_blank = False
        elif stripped == '':
            if not prev_blank:
                new_lines.append('')
                prev_blank = True
        else:
            new_lines.append(line)
            prev_blank = False

    return new_lines


def format_spacing(code: str) -> str:
    lines = code.splitlines()
    formatted_lines = ensure_single_blank_lines(lines)
    return '\n'.join(formatted_lines)