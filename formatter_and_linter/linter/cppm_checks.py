#!/usr/bin/env python3
"""
Module Interface (.cppm) Checks

Detects non-trivial function implementations inside .cppm module interfaces;
implementations belong in .cpp files.
"""

import re
from typing import List, Tuple

MAX_CPPM_INLINE_BODY_LINES = 1

CPPM_FUNCTION_SIGNATURE_PATTERN = re.compile(
    r'^\s*(?:export\s+)?(?:constexpr\s+|consteval\s+|constinit\s+|inline\s+|static\s+|'
    r'virtual\s+|explicit\s+|friend\s+|extern\s+)*'
    r'[\w:<>,~*&\s]+\s+([\w:~]+)\s*\([^;{}]*\)\s*'
    r'(?:const\s*)?(?:noexcept(?:\s*\([^)]*\))?\s*)?(?:->\s*[\w:<>,~*&\s]+)?\s*\{'
)

CONTROL_KEYWORDS = {"if", "for", "while", "switch", "catch"}


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
