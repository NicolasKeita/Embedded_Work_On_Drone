#!/usr/bin/env python3
"""
Comment detection and handling utilities for the C++ code formatter.

Provides functions for detecting, removing, and restoring comments in C++ code.
"""

import re
from typing import Tuple, List, Set


COMMENT_PLACEHOLDER = "___COMMENT_{}___"


def remove_comments(code: str) -> Tuple[str, List[str]]:
    comments = []

    def save_multiline_comment(match: re.Match) -> str:
        comments.append(match.group(0))
        return COMMENT_PLACEHOLDER.format(len(comments) - 1)

    code = re.sub(r'/\*.*?\*/', save_multiline_comment, code, flags=re.DOTALL)

    def save_singleline_comment(match: re.Match) -> str:
        comments.append(match.group(0))
        return COMMENT_PLACEHOLDER.format(len(comments) - 1)

    code = re.sub(r'//.*?$', save_singleline_comment, code, flags=re.MULTILINE)

    return code, comments


def restore_comments(code: str, comments: List[str]) -> str:
    for i, comment in enumerate(comments):
        code = code.replace(COMMENT_PLACEHOLDER.format(i), comment)
    return code


def detect_comments_and_functions(code: str) -> Tuple[List[Tuple[int, str]], Set[int]]:
    lines = code.splitlines()
    comments = []
    function_lines = set()

    scoped_name = r'(?:\w+::)*\w+'
    function_patterns = [
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'[\w:]+\s+' + scoped_name + r'\s*\([^)]*\)\s*(?:const\s*)?(?:final\s*|override\s*)?\s*(?:\{|$)',
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'(?:[\w:]+\s*[*&]\s+)+' + scoped_name + r'\s*\([^)]*\)\s*(?:const\s*)?(?:final\s*|override\s*)?\s*(?:\{|$)',
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'(?:[\w:]+\s*[*&]?\s+)+' + scoped_name + r'\s*\([^)]*\)\s*(?:const\s*)?(?:final\s*|override\s*)?\s*(?:\{|$)',
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'\w+\s*::\s*\w+\s*\([^)]*\)\s*(?:const\s*)?(?:final\s*|override\s*)?\s*(?:\{|$)',
        r'^\s*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'\w+\s*::\s*operator\s*=\s*\([^)]*\)\s*(?:const\s*)?(?:final\s*|override\s*)?\s*(?:\{|$)'
    ]

    for pattern_str in function_patterns:
        pattern = re.compile(pattern_str, re.MULTILINE)
        for match in pattern.finditer(code):
            line_num = code[:match.start()].count('\n') + 1
            line_content = match.group().strip()
            if 'curl_easy_setopt' not in line_content:
                function_lines.add(line_num)

    raw_string_pattern = re.compile(r'R"([^()]*)\((.*?)\)\1"', re.DOTALL)
    raw_string_ranges = []
    for match in raw_string_pattern.finditer(code):
        raw_string_ranges.append((match.start(), match.end()))

    string_pattern = re.compile(r'"(?:[^"\\]|\\.)*"')
    string_ranges = []
    for match in string_pattern.finditer(code):
        string_ranges.append((match.start(), match.end()))

    def is_in_string(pos):
        for start, end in raw_string_ranges:
            if start <= pos < end:
                return True
        for start, end in string_ranges:
            if start <= pos < end:
                return True
        return False

    singleline_pattern = re.compile(r'//.*$', re.MULTILINE)
    for match in singleline_pattern.finditer(code):
        if is_in_string(match.start()):
            continue
        line_num = code[:match.start()].count('\n') + 1
        line_content = match.group(0).strip()
        if line_content.startswith('//'):
            comments.append((line_num, "singleline"))
        else:
            comments.append((line_num, "inline"))

    multiline_pattern = re.compile(r'/\*.*?\*/', re.DOTALL)
    for match in multiline_pattern.finditer(code):
        start_line = code[:match.start()].count('\n') + 1
        end_line = code[:match.end()].count('\n') + 1
        comments.append((start_line, "multiline_start"))
        if start_line != end_line:
            for line_num in range(start_line + 1, end_line):
                comments.append((line_num, "multiline_content"))
        if end_line != start_line:
            comments.append((end_line, "multiline_end"))

    return comments, function_lines


def check_comment_placement(comments: List[Tuple[int, str]], function_lines: Set[int]) -> List[Tuple[int, str]]:
    invalid_comments = []

    comment_blocks = []
    current_block = []

    for line_num, comment_type in sorted(comments):
        if comment_type in ["singleline", "inline"]:
            if current_block:
                comment_blocks.append(current_block)
                current_block = []
            comment_blocks.append([(line_num, comment_type)])
        elif comment_type == "multiline_start":
            if current_block:
                comment_blocks.append(current_block)
            current_block = [(line_num, comment_type)]
        elif comment_type in ["multiline_content", "multiline_end"]:
            current_block.append((line_num, comment_type))

    if current_block:
        comment_blocks.append(current_block)

    for block in comment_blocks:
        if not block:
            continue
        first_line = block[0][0]
        last_line = block[-1][0]
        first_type = block[0][1]

        if first_type in ["singleline", "inline"]:
            if first_type == "inline":
                invalid_comments.extend(block)
            else:
                if (first_line + 1) not in function_lines:
                    is_header_comment = False
                    if first_line <= 3:
                        if function_lines and min(function_lines) > first_line:
                            is_header_comment = True
                    is_inside_function = False
                    if function_lines:
                        previous_functions = [f for f in function_lines if f < first_line]
                        if previous_functions:
                            next_functions = [f for f in function_lines if f > first_line]
                            if not next_functions:
                                is_inside_function = True
                    if not is_header_comment and not is_inside_function:
                        invalid_comments.extend(block)
                    elif is_inside_function:
                        invalid_comments.extend(block)
        elif first_type == "multiline_start":
            if (last_line + 1) not in function_lines:
                is_header_comment = False
                if first_line == 1:
                    if not function_lines or (function_lines and min(function_lines) > last_line):
                        is_header_comment = True
                is_inside_function = False
                if function_lines and last_line > 1:
                    previous_functions = [f for f in function_lines if f < first_line]
                    if previous_functions:
                        next_functions = [f for f in function_lines if f > last_line]
                        if not next_functions:
                            is_inside_function = True
                if not is_header_comment and not is_inside_function:
                    invalid_comments.extend(block)
                elif is_inside_function:
                    invalid_comments.extend(block)

    return invalid_comments