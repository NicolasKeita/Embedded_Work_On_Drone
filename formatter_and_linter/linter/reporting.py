#!/usr/bin/env python3
"""
Linter Reporting

Warning message rendering for the C++ code linter.
"""

import sys
from typing import List, Tuple

from linter.style_checks import MAX_FILE_LENGTH, MAX_FUNCTION_LENGTH
from linter.comment_language_checks import LINGUA_AVAILABLE
from linter.multiple_var_decl_checks import MULTIPLE_VAR_DECL_MESSAGE
from linter.main_filename_checks import MAIN_FILENAME_MESSAGE
from linter.module_size_checks import format_module_too_large_message
from linter.uninitialized_decl_checks import format_uninitialized_decl_message
from linter.designated_init_checks import format_designated_init_message
from linter.return_only_var_checks import format_return_only_var_message
from linter.cppm_inline_function_checks import format_cppm_inline_function_message


def _get_path_label(file_path: str) -> str:
    if file_path:
        return file_path
    return "<stdin>"


def print_issue_header(file_path: str) -> None:
    path_label = _get_path_label(file_path)
    print(f"\n{path_label}", file=sys.stderr)


def print_line_length_warnings(long_lines: List[Tuple[int, int]], max_length: int = 120) -> None:
    for line_num, actual_length in long_lines:
        print(f"⚠️  Line {line_num} exceeds {max_length} characters: {actual_length} characters", file=sys.stderr)


def print_comment_placement_warnings(invalid_comments: List[Tuple[int, str]]) -> None:
    for line_num, comment_type in invalid_comments:
        if comment_type == "singleline":
            print(f"⚠️  Line {line_num}: Single-line comment not above a function or declaration", file=sys.stderr)
        elif comment_type == "inline":
            print(f"⚠️  Line {line_num}: Inline comment not allowed (only comments above functions or declarations are permitted)", file=sys.stderr)
        elif comment_type == "multiline_start":
            print(f"⚠️  Line {line_num}: Multiline comment not above a function or declaration", file=sys.stderr)
        elif comment_type == "multiline_content":
            print(f"⚠️  Line {line_num}: Multiline comment content not above a function or declaration", file=sys.stderr)


def print_file_length_warning(exceeds_limit: bool, line_count: int, max_lines: int = MAX_FILE_LENGTH) -> None:
    if exceeds_limit:
        print(
            f"⚠️  File exceeds {max_lines} lines: {line_count} lines. "
            "Split into multiple smaller files with explicit, coherent names.",
            file=sys.stderr
        )


def print_function_length_warnings(long_functions: List[Tuple[str, int, int]], max_lines: int = MAX_FUNCTION_LENGTH) -> None:
    for func_name, start_line, line_count in long_functions:
        print(f"⚠️  Function '{func_name}' at line {start_line} exceeds {max_lines} lines: {line_count} lines", file=sys.stderr)


def print_comment_language_warnings(violations: List[Tuple[int, str]]) -> None:
    for line_num, detected_language in violations:
        print(f"⚠️  Line {line_num}: Comment must be written in English (detected: {detected_language})", file=sys.stderr)


def print_directory_file_count_warnings(
    violations: List[Tuple[str, int]],
    max_files: int
) -> None:
    for directory, file_count in violations:
        print(
            f"⚠️  Directory '{directory}' contains {file_count} source files (max: {max_files}). "
            "Split it into subdirectories.",
            file=sys.stderr
        )


def print_module_filename_warnings(violations: List[Tuple[str, str]]) -> None:
    for file_path, message in violations:
        print(f"⚠️  '{file_path}': {message}", file=sys.stderr)


def print_module_size_warnings(violations: List[Tuple[str, int, str]]) -> None:
    for label, file_count, suggestion in violations:
        print(
            f"⚠️  {format_module_too_large_message(label, file_count, suggestion)}",
            file=sys.stderr,
        )


def print_blank_line_after_initialization_warnings(
    violations: List[Tuple[str, int, int]],
) -> None:
    for func_name, func_start, last_decl_line in violations:
        print(
            f"⚠️  Function '{func_name}' at line {func_start}: missing blank line "
            f"after first declaration block (last declaration at line {last_decl_line})",
            file=sys.stderr,
        )


def print_multiple_var_decl_warnings(violations: List[int]) -> None:
    for line_num in violations:
        print(f"⚠️  Line {line_num}: {MULTIPLE_VAR_DECL_MESSAGE}", file=sys.stderr)


def print_main_filename_warnings(violations: List[int]) -> None:
    for line_num in violations:
        print(f"⚠️  Line {line_num}: {MAIN_FILENAME_MESSAGE}", file=sys.stderr)


def print_uninitialized_decl_warnings(violations: List[Tuple[int, str]]) -> None:
    for line_num, variable_name in violations:
        message = format_uninitialized_decl_message(variable_name)
        print(f"⚠️  Line {line_num}: {message}", file=sys.stderr)


def print_designated_init_warnings(
    violations: List[Tuple[int, str, str, List[str]]],
) -> None:
    for line_num, type_name, variable_name, field_chains in violations:
        message = format_designated_init_message(type_name, variable_name, field_chains)
        print(f"⚠️  Line {line_num}: {message}", file=sys.stderr)


def print_return_only_var_warnings(violations: List[Tuple[int, str]]) -> None:
    for line_num, variable_name in violations:
        message = format_return_only_var_message(variable_name)
        print(f"⚠️  Line {line_num}: {message}", file=sys.stderr)


def print_cppm_interface_warnings(
    violations: List[Tuple[str, int, int]],
    file_path: str
) -> None:
    path_label = _get_path_label(file_path)
    for function_name, start_line, body_line_count in violations:
        print(
            f"??  {path_label}:{start_line}: Non-trivial implementation in .cppm for "
            f"'{function_name}' ({body_line_count} body lines). Move it to a .cpp file.",
            file=sys.stderr
        )


def print_cppm_inline_function_warnings(
    violations: List[Tuple[str, int, int]],
    file_path: str
) -> None:
    path_label = _get_path_label(file_path)
    for function_name, start_line, body_lines in violations:
        message = format_cppm_inline_function_message(
            path_label, function_name, start_line, body_lines
        )
        print(f"⚠️  {message}", file=sys.stderr)
