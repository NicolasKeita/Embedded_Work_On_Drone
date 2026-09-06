#!/usr/bin/env python3
"""
Main Function Filename Checks

Ensures that any source file defining a main() entry point is named exactly
'main.cpp'. Occurrences of main() inside comments, strings or preprocessor
lines are ignored. When no file path is available (stdin), the filename
cannot be verified and the check is skipped.
"""

import os
import re
from typing import List

from linter.style_checks import _mask_strings_and_comments

MAIN_FILENAME_MESSAGE = "[MAIN_FILENAME] A file defining main() must be named 'main.cpp'."

_MAIN_FUNCTION_DEFINITION_RE = re.compile(r'^[A-Za-z_][\w:<>,&*\s]*?[\s*&]main\s*\(')


def _defines_main_function(code: str) -> List[int]:
    """
    Return the 1-based line numbers of main() function definitions in the
    file. Strings and comments are masked before matching, and preprocessor
    lines are skipped, so only real definitions are reported.
    """
    lines = code.splitlines()
    main_lines: List[int] = []
    in_block_comment = False

    for line_index, raw_line in enumerate(lines):
        masked_line, in_block_comment = _mask_strings_and_comments(raw_line, in_block_comment)
        stripped = masked_line.strip()
        if not stripped or stripped.startswith('#'):
            continue
        if _MAIN_FUNCTION_DEFINITION_RE.match(stripped):
            main_lines.append(line_index + 1)

    return main_lines


def check_main_function_filename(code: str, file_path: str) -> List[int]:
    """
    Report the lines defining a main() function when the file is not named
    exactly 'main.cpp'. Returns an empty list when the filename is correct,
    when the file does not define main() or when no file path is available
    to verify the name.
    """
    if not file_path:
        return []

    main_lines = _defines_main_function(code)
    if not main_lines:
        return []

    if os.path.basename(file_path) == "main.cpp":
        return []

    return main_lines
