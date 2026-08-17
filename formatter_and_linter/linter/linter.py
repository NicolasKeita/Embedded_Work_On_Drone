#!/usr/bin/env python3
"""
C++ Code Linter

Checks for style violations and code quality issues including line length,
comment placement, function length, and file length.
"""

import sys
import re
from typing import List, Tuple

from shared.comment_utils import detect_comments_and_functions, check_comment_placement

MAX_FUNCTION_LENGTH = 35
MAX_CPPM_INLINE_BODY_LINES = 1

CPPM_FUNCTION_SIGNATURE_PATTERN = re.compile(
    r'^\s*(?:export\s+)?(?:constexpr\s+|consteval\s+|constinit\s+|inline\s+|static\s+|'
    r'virtual\s+|explicit\s+|friend\s+|extern\s+)*'
    r'[\w:<>,~*&\s]+\s+([\w:~]+)\s*\([^;{}]*\)\s*'
    r'(?:const\s*)?(?:noexcept(?:\s*\([^)]*\))?\s*)?(?:->\s*[\w:<>,~*&\s]+)?\s*\{'
)

CONTROL_KEYWORDS = {"if", "for", "while", "switch", "catch"}


def _get_path_label(file_path: str) -> str:
    if file_path:
        return file_path
    return "<stdin>"


def print_issue_header(file_path: str) -> None:
    path_label = _get_path_label(file_path)
    print(f"\n{path_label}", file=sys.stderr)


def check_line_length(code: str, max_length: int = 120) -> List[Tuple[int, int]]:
    lines = code.splitlines()
    long_lines = []
    for i, line in enumerate(lines, 1):
        if len(line) > max_length:
            long_lines.append((i, len(line)))
    return long_lines


def print_line_length_warnings(long_lines: List[Tuple[int, int]], max_length: int = 120) -> None:
    for line_num, actual_length in long_lines:
        print(f"⚠️  Line {line_num} exceeds {max_length} characters: {actual_length} characters", file=sys.stderr)


def print_comment_placement_warnings(invalid_comments: List[Tuple[int, str]]) -> None:
    for line_num, comment_type in invalid_comments:
        if comment_type == "singleline":
            print(f"⚠️  Line {line_num}: Single-line comment not above a function", file=sys.stderr)
        elif comment_type == "inline":
            print(f"⚠️  Line {line_num}: Inline comment not allowed (only comments above functions are permitted)", file=sys.stderr)
        elif comment_type == "multiline_start":
            print(f"⚠️  Line {line_num}: Multiline comment not above a function", file=sys.stderr)
        elif comment_type == "multiline_content":
            print(f"⚠️  Line {line_num}: Multiline comment content not above a function", file=sys.stderr)


def check_file_length(code: str, max_lines: int = 120) -> Tuple[bool, int]:
    line_count = len(code.splitlines())
    return (line_count > max_lines, line_count)


def print_file_length_warning(exceeds_limit: bool, line_count: int, max_lines: int = 120) -> None:
    if exceeds_limit:
        print(f"⚠️  File exceeds {max_lines} lines: {line_count} lines", file=sys.stderr)


def check_function_length(code: str, max_lines: int = MAX_FUNCTION_LENGTH) -> List[Tuple[str, int, int]]:
    lines = code.splitlines()
    long_functions = []
    brace_stack = []
    current_depth = 0

    function_pattern = re.compile(
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+)*'
        r'[\w:]+\s*[*&]?\s+'
        r'(\w+(?:::\w+)?)\s*'
        r'\([^)]*\)\s*'
        r'(?:const\s*)?(?:final\s*|override\s*)?(?:\{|$)'
    )

    pending_function = None

    for i, line in enumerate(lines, 1):
        match = function_pattern.search(line)
        if match:
            func_name = match.group(1)
            if '{' in line:
                brace_stack.append((current_depth, func_name, i))
                current_depth += 1
            else:
                pending_function = (func_name, i)

        if pending_function and line.strip() == '{':
            func_name, line_num = pending_function
            brace_stack.append((current_depth, func_name, line_num))
            current_depth += 1
            pending_function = None
            continue

        open_braces = line.count('{')
        close_braces = line.count('}')

        braces_to_count = open_braces
        if match and '{' in line:
            braces_to_count -= 1

        for _ in range(braces_to_count):
            current_depth += 1

        if close_braces > 0:
            for _ in range(close_braces):
                current_depth -= 1
                if brace_stack and current_depth == brace_stack[-1][0]:
                    _, func_name, start_line = brace_stack.pop()
                    func_lines = i - start_line + 1
                    if func_lines > max_lines:
                        long_functions.append((func_name, start_line, func_lines))

    return long_functions


def print_function_length_warnings(long_functions: List[Tuple[str, int, int]], max_lines: int = MAX_FUNCTION_LENGTH) -> None:
    for func_name, start_line, line_count in long_functions:
        print(f"⚠️  Function '{func_name}' at line {start_line} exceeds {max_lines} lines: {line_count} lines", file=sys.stderr)


def _build_line_depths(lines: List[str]) -> List[int]:
    depths = []
    current_depth = 0
    for line in lines:
        depths.append(current_depth)
        current_depth += line.count('{')
        current_depth -= line.count('}')
        if current_depth < 0:
            current_depth = 0
    return depths


def _count_significant_body_lines(lines: List[str], open_line: int, end_line: int) -> int:
    significant_lines = 0
    for line_idx in range(open_line, end_line + 1):
        body_line = lines[line_idx]
        if line_idx == open_line:
            opening_pos = body_line.find('{')
            if opening_pos >= 0:
                body_line = body_line[opening_pos + 1:]
        if line_idx == end_line:
            closing_pos = body_line.rfind('}')
            if closing_pos >= 0:
                body_line = body_line[:closing_pos]

        stripped = body_line.strip()
        if not stripped:
            continue
        if stripped in {'{', '}'}:
            continue
        significant_lines += 1

    return significant_lines


def check_cppm_interface_implementations(
    code: str,
    max_body_lines: int = MAX_CPPM_INLINE_BODY_LINES
) -> List[Tuple[str, int, int]]:
    lines = code.splitlines()
    has_class_declaration = any(re.search(r'^\s*(?:export\s+)?class\s+\w+', line) for line in lines)
    if not has_class_declaration:
        return []

    line_depths = _build_line_depths(lines)
    violations = []

    i = 0
    while i < len(lines):
        if line_depths[i] != 0:
            i += 1
            continue

        if '(' not in lines[i]:
            i += 1
            continue

        signature_lines = []
        first_brace_line = -1
        scan_idx = i

        while scan_idx < len(lines) and scan_idx < i + 12:
            signature_lines.append(lines[scan_idx].strip())
            current_line = lines[scan_idx]

            if ';' in current_line and '{' not in current_line:
                break

            if '{' in current_line:
                first_brace_line = scan_idx
                break

            scan_idx += 1

        if first_brace_line == -1:
            i += 1
            continue

        normalized_signature = ' '.join(signature_lines)
        signature_match = CPPM_FUNCTION_SIGNATURE_PATTERN.match(normalized_signature)
        if not signature_match:
            i += 1
            continue

        function_name = signature_match.group(1).split('::')[-1]
        if function_name in CONTROL_KEYWORDS:
            i += 1
            continue

        local_depth = 0
        end_line = -1
        for end_idx in range(first_brace_line, len(lines)):
            local_depth += lines[end_idx].count('{')
            local_depth -= lines[end_idx].count('}')
            if local_depth == 0:
                end_line = end_idx
                break

        if end_line == -1:
            i += 1
            continue

        significant_lines = _count_significant_body_lines(lines, first_brace_line, end_line)
        if significant_lines > max_body_lines:
            full_name = signature_match.group(1)
            violations.append((full_name, i + 1, significant_lines))

        i = end_line + 1

    return violations


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


def lint_code(code: str, max_length: int = 120, file_path: str = "") -> bool:
    is_module_interface = file_path.lower().endswith('.cppm')
    if is_module_interface:
        cppm_violations = check_cppm_interface_implementations(code)
        if cppm_violations:
            print_issue_header(file_path)
        print_cppm_interface_warnings(cppm_violations, file_path)
        return len(cppm_violations) > 0

    long_lines = check_line_length(code, max_length)
    comments, function_lines = detect_comments_and_functions(code)
    invalid_comments = check_comment_placement(comments, function_lines)
    long_functions = check_function_length(code, max_lines=MAX_FUNCTION_LENGTH)
    file_too_long, file_line_count = check_file_length(code, max_lines=120)
    has_issues = (
        len(long_lines) > 0
        or len(invalid_comments) > 0
        or len(long_functions) > 0
        or file_too_long
    )
    if has_issues:
        print_issue_header(file_path)
        print_line_length_warnings(long_lines, max_length)
        print_comment_placement_warnings(invalid_comments)
        print_function_length_warnings(long_functions, max_lines=MAX_FUNCTION_LENGTH)
        print_file_length_warning(file_too_long, file_line_count, max_lines=120)

    return has_issues


if __name__ == "__main__":
    code = sys.stdin.read()
    has_issues = lint_code(code)
    sys.exit(1 if has_issues else 0)