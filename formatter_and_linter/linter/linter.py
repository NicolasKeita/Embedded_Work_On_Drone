#!/usr/bin/env python3
"""
C++ Code Linter

Checks for style violations and code quality issues including line length,
comment placement, comment language, function length, and file length.

The implementation lives in style_checks, comment_language_checks,
cppm_checks and reporting.
"""

import sys

from shared.comment_utils import detect_comments_and_functions, check_comment_placement

from linter.style_checks import (
    MAX_FILE_LENGTH,
    MAX_FUNCTION_LENGTH,
    check_line_length,
    check_file_length,
    check_function_length,
)
from linter.cppm_checks import (
    MAX_CPPM_INLINE_BODY_LINES,
    check_cppm_interface_implementations,
)
from linter.comment_language_checks import check_comment_language
from linter.reporting import (
    _get_path_label,
    print_issue_header,
    print_line_length_warnings,
    print_comment_placement_warnings,
    print_comment_language_warnings,
    print_file_length_warning,
    print_function_length_warnings,
    print_cppm_interface_warnings,
)


def lint_code(code: str, max_length: int = 120, file_path: str = "") -> bool:
    is_module_interface = file_path.lower().endswith('.cppm')
    if is_module_interface:
        cppm_violations = check_cppm_interface_implementations(code)
        file_too_long, file_line_count = check_file_length(code, max_lines=MAX_FILE_LENGTH)
        language_violations = check_comment_language(code)

        has_issues = (
            len(cppm_violations) > 0
            or file_too_long
            or len(language_violations) > 0
        )
        if has_issues:
            print_issue_header(file_path)

        print_cppm_interface_warnings(cppm_violations, file_path)
        print_file_length_warning(file_too_long, file_line_count, max_lines=MAX_FILE_LENGTH)
        print_comment_language_warnings(language_violations)
        return has_issues

    long_lines = check_line_length(code, max_length)
    comments, function_lines = detect_comments_and_functions(code)
    invalid_comments = check_comment_placement(comments, function_lines)
    long_functions = check_function_length(code, max_lines=MAX_FUNCTION_LENGTH)
    file_too_long, file_line_count = check_file_length(code, max_lines=MAX_FILE_LENGTH)
    language_violations = check_comment_language(code)
    has_issues = (
        len(long_lines) > 0
        or len(invalid_comments) > 0
        or len(long_functions) > 0
        or file_too_long
        or len(language_violations) > 0
    )
    if has_issues:
        print_issue_header(file_path)
        print_line_length_warnings(long_lines, max_length)
        print_comment_placement_warnings(invalid_comments)
        print_function_length_warnings(long_functions, max_lines=MAX_FUNCTION_LENGTH)
        print_file_length_warning(file_too_long, file_line_count, max_lines=MAX_FILE_LENGTH)
        print_comment_language_warnings(language_violations)

    return has_issues


__all__ = [
    "MAX_FUNCTION_LENGTH",
    "MAX_FILE_LENGTH",
    "MAX_CPPM_INLINE_BODY_LINES",
    "check_line_length",
    "check_file_length",
    "check_function_length",
    "check_cppm_interface_implementations",
    "_get_path_label",
    "print_issue_header",
    "print_line_length_warnings",
    "print_comment_placement_warnings",
    "print_comment_language_warnings",
    "print_file_length_warning",
    "print_function_length_warnings",
    "print_cppm_interface_warnings",
    "lint_code",
]


if __name__ == "__main__":
    code = sys.stdin.read()
    has_issues = lint_code(code)
    sys.exit(1 if has_issues else 0)
