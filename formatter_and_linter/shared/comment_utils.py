#!/usr/bin/env python3
"""
Comment detection and handling utilities for the C++ code formatter.

Provides functions for detecting, removing, and restoring comments in C++ code.
"""

import re
from typing import Tuple, List, Set


COMMENT_PLACEHOLDER = "___COMMENT_{}___"


def _raw_string_prefix_start(code: str, quote_index: int) -> int:
    if quote_index <= 0 or code[quote_index - 1] != 'R':
        return -1
    r_index = quote_index - 1
    before_r = r_index - 1
    if before_r < 0:
        return r_index
    previous = code[before_r]
    if previous in ('u', 'U', 'L'):
        before_prefix = before_r - 1
        if before_prefix >= 0 and (code[before_prefix].isalnum() or code[before_prefix] == '_'):
            return -1
        return before_r
    if previous == '8' and before_r - 1 >= 0 and code[before_r - 1] == 'u':
        before_prefix = before_r - 2
        if before_prefix >= 0 and (code[before_prefix].isalnum() or code[before_prefix] == '_'):
            return -1
        return before_r - 1
    if previous.isalnum() or previous == '_':
        return -1
    return r_index


def scan_string_and_comment_ranges(code: str) -> Tuple[List[Tuple[int, int]], List[Tuple[int, int]]]:
    length = len(code)
    string_ranges: List[Tuple[int, int]] = []
    comment_ranges: List[Tuple[int, int]] = []
    index = 0
    while index < length:
        char = code[index]
        following = code[index + 1] if index + 1 < length else ''
        if char == '/' and following == '/':
            end = code.find('\n', index)
            if end == -1:
                end = length
            comment_ranges.append((index, end))
            index = end
            continue
        if char == '/' and following == '*':
            end = code.find('*/', index + 2)
            if end == -1:
                end = length
            else:
                end += 2
            comment_ranges.append((index, end))
            index = end
            continue
        if char == '"':
            prefix_start = _raw_string_prefix_start(code, index)
            if prefix_start != -1:
                delimiter_end = code.find('(', index + 1, index + 17)
                delimiter = code[index + 1:delimiter_end] if delimiter_end != -1 else ''
                if delimiter_end != -1 and not any(item in ' \\()' for item in delimiter):
                    terminator = ')' + delimiter + '"'
                    term_index = code.find(terminator, delimiter_end + 1)
                    if term_index != -1:
                        end = term_index + len(terminator)
                        string_ranges.append((prefix_start, end))
                        index = end
                        continue
            closing = index + 1
            closed = False
            while closing < length and code[closing] != '\n':
                if code[closing] == '\\':
                    closing += 2
                    continue
                if code[closing] == '"':
                    closed = True
                    break
                closing += 1
            if closed:
                string_ranges.append((index, closing + 1))
                index = closing + 1
            else:
                index += 1
            continue
        if char == "'":
            closing = index + 1
            if closing < length and code[closing] == '\\':
                closing += 2
            else:
                closing += 1
            if closing < length and code[closing] == "'" and '\n' not in code[index:closing + 1]:
                string_ranges.append((index, closing + 1))
                index = closing + 1
            else:
                index += 1
            continue
        index += 1
    return string_ranges, comment_ranges


def remove_comments(code: str) -> Tuple[str, List[str]]:
    comments = []
    string_ranges, comment_ranges = scan_string_and_comment_ranges(code)
    comment_ranges = sorted(comment_ranges)
    parts: List[str] = []
    cursor = 0
    for start, end in comment_ranges:
        parts.append(code[cursor:start])
        parts.append(COMMENT_PLACEHOLDER.format(len(comments)))
        comments.append(code[start:end])
        cursor = end
    parts.append(code[cursor:])
    return ''.join(parts), comments


def restore_comments(code: str, comments: List[str]) -> str:
    for i, comment in enumerate(comments):
        code = code.replace(COMMENT_PLACEHOLDER.format(i), comment)
    return code


def detect_comments_and_functions(code: str) -> Tuple[List[Tuple[int, str]], Set[int], Set[int]]:
    comments = []
    function_lines = set()
    declaration_lines = set()

    scoped_name = r'(?:\w+::)*\w+'
    template_type = r'[\w:]+(?:\s*<(?:[^<>]|<(?:[^<>]|<[^<>]*>)*>)*>)?'
    function_suffix = (
        r'\s*'
        r'(?:(?:const|final|override)\s*|noexcept\s*(?:\([^)]*\)\s*)?)*'
        r'(?:->\s*[\w:<>,&*\s]+?\s*)?'
        r'(?:\s*:\s*[^{;}]+?)?'
        r'\s*(?:\{|$)'
    )
    attribute_specifier = r'(?:[ \t]*\[\[[^\]]*\]\])*[ \t]*'
    function_patterns = [
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + attribute_specifier + template_type + r'\s+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + attribute_specifier + r'(?:' + template_type + r'\s*[*&]\s+)+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + attribute_specifier + r'(?:' + template_type + r'\s*[*&]?\s+)+' + scoped_name + r'\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + attribute_specifier + r'\w+\s*::\s*\w+\s*\([^)]*\)' + function_suffix,
        r'^[ \t]*(?:static\s+|inline\s+|virtual\s+|explicit\s+|constexpr\s+|const\s+)*'
        + attribute_specifier + r'\w+\s*::\s*operator\s*=\s*\([^)]*\)' + function_suffix,
    ]

    declaration_pattern = re.compile(
        r'^[ \t]*(?:static[ \t]+|inline[ \t]+|constexpr[ \t]+|const[ \t]+|extern[ \t]+|mutable[ \t]+)*'
        + attribute_specifier +
        r'(?:' + template_type + r'[ \t]*[*&]?[ \t]+)+'
        r'\w+[ \t]*(?:\[[^\]]*\])?[ \t]*(?:=|\{|\(|;)',
        re.MULTILINE,
    )
    type_declaration_pattern = re.compile(r'^[ \t]*(?:struct|class|enum|union)[ \t]+\w+', re.MULTILINE)
    alias_declaration_pattern = re.compile(r'^[ \t]*using[ \t]+[\w:]+(?:\s*<(?:[^<>]|<(?:[^<>]|<[^<>]*>)*>)*>)?[ \t]*=', re.MULTILINE)
    excluded_declaration_pattern = re.compile(r'^[ \t]*(?:import[ \t]|export[ \t]|module[ \t])')

    string_ranges: List[Tuple[int, int]] = []
    comment_ranges: List[Tuple[int, int]] = []
    scanned_strings, scanned_comments = scan_string_and_comment_ranges(code)
    string_ranges = list(scanned_strings)
    comment_ranges = list(scanned_comments)

    def is_in_string(pos):
        for start, end in string_ranges:
            if start <= pos < end:
                return True
        return False

    def is_in_comment(pos):
        for start, end in comment_ranges:
            if start <= pos < end:
                return True
        return False

    masked_chars = list(code)
    for start, end in string_ranges + comment_ranges:
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

    template_prefix_pattern = re.compile(r'^[ \t]*template\s*<', re.MULTILINE)
    for match in template_prefix_pattern.finditer(masked_code):
        if is_in_string(match.start()) or is_in_comment(match.start()):
            continue
        line_num = code[:match.start()].count('\n') + 1
        if (line_num + 1) in function_lines or (line_num + 1) in declaration_lines:
            function_lines.add(line_num)

    for start, end in comment_ranges:
        start_line = code[:start].count('\n') + 1
        end_line = code[:end].count('\n') + 1
        if code[start:start + 2] == '//':
            line_num = start_line
            line_start = code.rfind('\n', 0, start) + 1
            if code[line_start:start].strip():
                comments.append((line_num, "inline"))
            else:
                comments.append((line_num, "singleline"))
            continue
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