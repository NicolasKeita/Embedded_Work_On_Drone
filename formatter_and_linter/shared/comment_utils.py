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


def detect_comments_and_functions(code: str) -> Tuple[List[Tuple[int, str]], Set[int], Set[int]]:
    comments = []
    function_lines = set()
    declaration_lines = set()

    scoped_name = r'(?:\w+::)*\w+'
    template_type = r'[\w:]+(?:<[^<>]*>)?'
    function_suffix = (
        r'\s*'
        r'(?:(?:const|final|override)\s*|noexcept\s*(?:\([^)]*\)\s*)?)*'
        r'(?:->\s*[\w:<>,&*\s]+?\s*)?'
        r'(?:\s*:\s*[^{;}]+?)?'
        r'\s*(?:\{|$)'
    )
    function_patterns = [
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + template_type + r'\s+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'(?:' + template_type + r'\s*[*&]\s+)+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'(?:' + template_type + r'\s*[*&]?\s+)+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'\w+\s*::\s*\w+\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        r'\w+\s*::\s*operator\s*=\s*\([^)]*\)' + function_suffix,
    ]

    declaration_pattern = re.compile(
        r'^[ \t]*(?:static[ \t]+|inline[ \t]+|constexpr[ \t]+|const[ \t]+|extern[ \t]+|mutable[ \t]+)*'
        r'(?:' + template_type + r'[ \t]*[*&]?[ \t]+)+'
        r'\w+[ \t]*(?:\[[^\]]*\])?[ \t]*(?:=|\{|\(|;)',
        re.MULTILINE,
    )
    type_declaration_pattern = re.compile(r'^[ \t]*(?:struct|class|enum|union)[ \t]+\w+', re.MULTILINE)
    alias_declaration_pattern = re.compile(r'^[ \t]*using[ \t]+[\w:]+(?:<[^<>]*>)?[ \t]*=', re.MULTILINE)
    excluded_declaration_pattern = re.compile(r'^[ \t]*(?:import[ \t]|export[ \t]|module[ \t])')

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

    multiline_pattern = re.compile(r'/\*.*?\*/', re.DOTALL)
    singleline_pattern = re.compile(r'//.*$', re.MULTILINE)

    comment_ranges = []
    def is_in_comment(pos):
        for start, end in comment_ranges:
            if start <= pos < end:
                return True
        return False

    for match in multiline_pattern.finditer(code):
        if is_in_string(match.start()):
            continue
        comment_ranges.append((match.start(), match.end()))
    for match in singleline_pattern.finditer(code):
        if is_in_string(match.start()) or is_in_comment(match.start()):
            continue
        comment_ranges.append((match.start(), match.end()))

    masked_chars = list(code)
    for start, end in raw_string_ranges + string_ranges + comment_ranges:
        for i in range(start, end):
            if masked_chars[i] != '\n':
                masked_chars[i] = ' '
    masked_code = ''.join(masked_chars)

    function_match_starts = []
    for pattern_str in function_patterns:
        pattern = re.compile(pattern_str, re.MULTILINE)
        for match in pattern.finditer(masked_code):
            if is_in_comment(match.start()):
                continue
            line_num = code[:match.start()].count('\n') + 1
            line_content = match.group().strip()
            if 'curl_easy_setopt' not in line_content:
                function_lines.add(line_num)
                function_match_starts.append(match.start())

    function_body_lines = _find_function_body_lines(masked_code, function_match_starts)

    for declaration_finder in (declaration_pattern, type_declaration_pattern, alias_declaration_pattern):
        for match in declaration_finder.finditer(masked_code):
            line_num = code[:match.start()].count('\n') + 1
            if line_num in function_lines or line_num in function_body_lines:
                continue
            line_text = _line_at(masked_code, match.start()).strip()
            if excluded_declaration_pattern.match(line_text) or line_text.startswith('#'):
                continue
            declaration_lines.add(line_num)

    for match in singleline_pattern.finditer(code):
        if is_in_string(match.start()):
            continue
        line_num = code[:match.start()].count('\n') + 1
        line_content = match.group(0).strip()
        if line_content.startswith('//'):
            comments.append((line_num, "singleline"))
        else:
            comments.append((line_num, "inline"))

    for match in multiline_pattern.finditer(code):
        if is_in_string(match.start()):
            continue
        start_line = code[:match.start()].count('\n') + 1
        end_line = code[:match.end()].count('\n') + 1
        comments.append((start_line, "multiline_start"))
        if start_line != end_line:
            for line_num in range(start_line + 1, end_line):
                comments.append((line_num, "multiline_content"))
        if end_line != start_line:
            comments.append((end_line, "multiline_end"))

    return comments, function_lines, declaration_lines


def _line_at(code: str, pos: int) -> str:
    line_start = code.rfind('\n', 0, pos) + 1
    line_end = code.find('\n', pos)
    if line_end == -1:
        line_end = len(code)
    return code[line_start:line_end]


def _find_function_body_lines(masked_code: str, match_starts: List[int]) -> Set[int]:
    body_lines = set()

    for start in match_starts:
        brace_idx = masked_code.find('{', start)
        semi_idx = masked_code.find(';', start)
        if brace_idx == -1 or (semi_idx != -1 and semi_idx < brace_idx):
            continue

        depth = 0
        end_idx = len(masked_code) - 1
        for i in range(brace_idx, len(masked_code)):
            char = masked_code[i]
            if char == '{':
                depth += 1
            elif char == '}':
                depth -= 1
                if depth == 0:
                    end_idx = i
                    break

        start_line = masked_code[:brace_idx].count('\n') + 1
        end_line = masked_code[:end_idx].count('\n') + 1
        body_lines.update(range(start_line, end_line + 1))

    return body_lines


def check_comment_placement(
    comments: List[Tuple[int, str]],
    function_lines: Set[int],
    declaration_lines: Set[int],
) -> List[Tuple[int, str]]:
    invalid_comments = []
    anchor_lines = function_lines | declaration_lines

    comment_blocks = []
    current_block = []

    for line_num, comment_type in sorted(comments):
        if comment_type == "singleline":
            is_contiguous = (
                current_block
                and current_block[-1][1] == "singleline"
                and current_block[-1][0] == line_num - 1
            )
            if is_contiguous:
                current_block.append((line_num, comment_type))
            else:
                if current_block:
                    comment_blocks.append(current_block)
                current_block = [(line_num, comment_type)]
        elif comment_type == "multiline_start":
            if current_block:
                comment_blocks.append(current_block)
            current_block = [(line_num, comment_type)]
        elif comment_type in ("multiline_content", "multiline_end"):
            current_block.append((line_num, comment_type))
        else:
            if current_block:
                comment_blocks.append(current_block)
                current_block = []
            comment_blocks.append([(line_num, comment_type)])

    if current_block:
        comment_blocks.append(current_block)

    for block in comment_blocks:
        if not block:
            continue
        first_line = block[0][0]
        last_line = block[-1][0]
        first_type = block[0][1]

        if first_type == "inline":
            invalid_comments.extend(block)
            continue

        if (last_line + 1) in anchor_lines:
            continue

        is_header_comment = first_line <= 3 and (
            not anchor_lines or min(anchor_lines) > last_line
        )

        if not is_header_comment:
            invalid_comments.extend(block)

    return invalid_comments