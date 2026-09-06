#!/usr/bin/env python3
"""
C++ Code Linter

Checks for style violations and code quality issues including line length,
comment placement, comment language, function length, file length, directory
file count and module implementation count.

The implementation lives in style_checks, comment_language_checks,
cppm_checks, cppm_inline_function_checks, directory_checks,
module_filename_checks, module_size_checks and reporting.
"""

import sys

from shared.comment_utils import detect_comments_and_functions, check_comment_placement

from linter.style_checks import (
    MAX_FILE_LENGTH,
    MAX_FUNCTION_LENGTH,
    check_line_length,
    check_file_length,
    check_function_length,
    check_blank_line_after_initialization,
)
from linter.cppm_checks import (
    MAX_CPPM_INLINE_BODY_LINES,
    check_cppm_interface_implementations,
)
from linter.cppm_inline_function_checks import (
    MAX_CPPM_INLINE_FUNCTION_BODY_LINES,
    check_cppm_inline_function_bodies,
    check_cppm_inline_functions,
    format_cppm_inline_function_message,
)
from linter.comment_language_checks import check_code_comments_language
from linter.multiple_var_decl_checks import check_multiple_var_declarations
from linter.uninitialized_decl_checks import check_uninitialized_declarations
from linter.designated_init_checks import check_designated_init_candidates
from linter.return_only_var_checks import check_return_only_variable
from linter.directory_checks import (
    MAX_FILES_PER_DIRECTORY,
    check_directory_file_counts,
)
from linter.module_filename_checks import check_module_filename_convention
from linter.module_size_checks import (
    MAX_IMPLEMENTATION_FILES_PER_MODULE,
    check_module_implementation_counts,
)
from linter.reporting import (
    _get_path_label,
    print_issue_header,
    print_line_length_warnings,
    print_comment_placement_warnings,
    print_comment_language_warnings,
    print_file_length_warning,
    print_function_length_warnings,
    print_blank_line_after_initialization_warnings,
    print_multiple_var_decl_warnings,
    print_uninitialized_decl_warnings,
    print_designated_init_warnings,
    print_return_only_var_warnings,
    print_cppm_interface_warnings,
    print_cppm_inline_function_warnings,
    print_directory_file_count_warnings,
    print_module_filename_warnings,
    print_module_size_warnings,
)


def lint_code(code: str, max_length: int = 120, file_path: str = "") -> bool:
    is_module_interface = file_path.lower().endswith('.cppm')
    if is_module_interface:
        cppm_violations = check_cppm_inline_function_bodies(code)
        file_too_long, file_line_count = check_file_length(code, max_lines=MAX_FILE_LENGTH)
        language_violations = check_code_comments_language(code)
        multiple_decl_violations = check_multiple_var_declarations(code)
        uninitialized_decl_violations = check_uninitialized_declarations(code)
        designated_init_violations = check_designated_init_candidates(code)
        return_only_var_violations = check_return_only_variable(code)

        has_issues = (
            len(cppm_violations) > 0
            or file_too_long
            or len(language_violations) > 0
            or len(multiple_decl_violations) > 0
            or len(uninitialized_decl_violations) > 0
            or len(designated_init_violations) > 0
            or len(return_only_var_violations) > 0
        )
        if has_issues:
            print_issue_header(file_path)
        print_cppm_inline_function_warnings(cppm_violations, file_path)
        print_file_length_warning(file_too_long, file_line_count, max_lines=MAX_FILE_LENGTH)
        print_comment_language_warnings(language_violations)
        print_multiple_var_decl_warnings(multiple_decl_violations)
        print_uninitialized_decl_warnings(uninitialized_decl_violations)
        print_designated_init_warnings(designated_init_violations)
        print_return_only_var_warnings(return_only_var_violations)

        return has_issues

    long_lines = check_line_length(code, max_length)
    comments, function_lines, declaration_lines = detect_comments_and_functions(code)
    invalid_comments = check_comment_placement(comments, function_lines, declaration_lines)
    long_functions = check_function_length(code, max_lines=MAX_FUNCTION_LENGTH)
    blank_line_violations = check_blank_line_after_initialization(code)
    file_too_long, file_line_count = check_file_length(code, max_lines=MAX_FILE_LENGTH)
    language_violations = check_code_comments_language(code)
    multiple_decl_violations = check_multiple_var_declarations(code)
    uninitialized_decl_violations = check_uninitialized_declarations(code)
    designated_init_violations = check_designated_init_candidates(code)
    return_only_var_violations = check_return_only_variable(code)
    has_issues = (
        len(long_lines) > 0
        or len(invalid_comments) > 0
        or len(long_functions) > 0
        or len(blank_line_violations) > 0
        or file_too_long
        or len(language_violations) > 0
        or len(multiple_decl_violations) > 0
        or len(uninitialized_decl_violations) > 0
        or len(designated_init_violations) > 0
        or len(return_only_var_violations) > 0
    )
    if has_issues:
        print_issue_header(file_path)
        print_line_length_warnings(long_lines, max_length)
        print_comment_placement_warnings(invalid_comments)
        print_function_length_warnings(long_functions, max_lines=MAX_FUNCTION_LENGTH)
        print_blank_line_after_initialization_warnings(blank_line_violations)
        print_file_length_warning(file_too_long, file_line_count, max_lines=MAX_FILE_LENGTH)
        print_comment_language_warnings(language_violations)
        print_multiple_var_decl_warnings(multiple_decl_violations)
        print_uninitialized_decl_warnings(uninitialized_decl_violations)
        print_designated_init_warnings(designated_init_violations)
        print_return_only_var_warnings(return_only_var_violations)

    return has_issues


__all__ = [
    "MAX_FUNCTION_LENGTH",
    "MAX_FILE_LENGTH",
    "MAX_CPPM_INLINE_BODY_LINES",
    "MAX_FILES_PER_DIRECTORY",
    "check_line_length",
    "check_file_length",
    "check_function_length",
    "check_blank_line_after_initialization",
    "check_multiple_var_declarations",
    "check_uninitialized_declarations",
    "check_designated_init_candidates",
    "check_return_only_variable",
    "check_cppm_interface_implementations",
    "MAX_CPPM_INLINE_FUNCTION_BODY_LINES",
    "check_cppm_inline_function_bodies",
    "check_cppm_inline_functions",
    "format_cppm_inline_function_message",
    "check_directory_file_counts",
    "check_module_filename_convention",
    "MAX_IMPLEMENTATION_FILES_PER_MODULE",
    "check_module_implementation_counts",
    "_get_path_label",
    "print_issue_header",
    "print_line_length_warnings",
    "print_comment_placement_warnings",
    "print_comment_language_warnings",
    "print_file_length_warning",
    "print_function_length_warnings",
    "print_blank_line_after_initialization_warnings",
    "print_multiple_var_decl_warnings",
    "print_uninitialized_decl_warnings",
    "print_designated_init_warnings",
    "print_return_only_var_warnings",
    "print_cppm_interface_warnings",
    "print_cppm_inline_function_warnings",
    "print_directory_file_count_warnings",
    "print_module_filename_warnings",
    "print_module_size_warnings",
    "lint_code",
]


if __name__ == "__main__":
    code = sys.stdin.read()
    has_issues = lint_code(code)
    sys.exit(1 if has_issues else 0)
