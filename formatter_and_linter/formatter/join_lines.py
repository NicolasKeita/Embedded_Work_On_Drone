#!/usr/bin/env python3
"""
Line Joining

A formatting pass that merges C++ statements that were wrapped over several
lines back onto a single line whenever the merged line still fits within the
maximum line length (120 characters by default, indentation included).

The pass is multi-pass: the document is scanned line by line and the operation
repeats until no further merge is possible.

Merging is driven by C++ continuation heuristics. Line N may end with a
continuation token ('=', ',', '(', '[', '{', '+', '-', '*', '/', '&&', '||',
'<<', '>>', ':', '->', '.') or line N+1 may start with a separator or closing
token (')', ']', '}', ';' or an operator). The indentation of line N is
preserved; the indentation and the line break of line N+1 are replaced by a
single space.

Lines are never merged when either side contains a single-line comment ('//'),
when either side is a preprocessor directive, or when the merged line would
exceed max_length. An opening brace that introduces a function, control-
structure, namespace, class, struct, union, enum or lambda header never
swallows the following statement, so the brace style produced by the rest of
the pipeline (Allman for function bodies, K&R for control structures) is
preserved.
"""

import re
from typing import List

_CONTROL_HEADER = re.compile(r"^\s*(?:if|else|for|while|switch|catch|do|try)\b")
_CONTROL_HEADER_LINE = re.compile(r"^\s*(?:if|else|for|while|switch|catch|do|try)\b(?:.*\))?\s*$")
_TYPE_HEADER = re.compile(r"^\s*(?:namespace|class|struct|union|enum)\b")
_OPEN_BRACE_HEADER = re.compile(r"(?:\)|\]|else|do|try)\s*\{$")

_END_TOKENS = ("&&", "||", "<<", ">>", "->")
_END_SINGLE = set("=,([{+-*/:.")

_START_TOKENS = ("&&", "||", "<<", ">>", "->")
_START_SINGLE = set("=,([{+-*/:.)];")
_UNARY_START = set("+-*&!~")


def _ends_with_trigger(text: str) -> bool:
    for token in _END_TOKENS:
        if text.endswith(token):
            return True
    return bool(text) and text[-1] in _END_SINGLE


def _starts_with_trigger(text: str) -> bool:
    for token in _START_TOKENS:
        if text.startswith(token):
            return True
    return bool(text) and text[0] in _START_SINGLE


def _has_line_comment(line: str) -> bool:
    in_string = False
    string_char = ""
    i = 0
    length = len(line)
    while i < length:
        char = line[i]
        if in_string:
            if char == "\\":
                i += 2
                continue
            if char == string_char:
                in_string = False
            i += 1
            continue
        if char in ('"', "'"):
            in_string = True
            string_char = char
            i += 1
            continue
        if char == "/" and i + 1 < length and line[i + 1] == "/":
            return True
        i += 1
    return False


def _block_comment_mask(lines: List[str]) -> List[bool]:
    mask = []
    in_block = False
    for line in lines:
        line_masked = in_block
        in_string = False
        string_char = ""
        i = 0
        length = len(line)
        while i < length:
            char = line[i]
            if in_block:
                if char == "*" and i + 1 < length and line[i + 1] == "/":
                    in_block = False
                    i += 2
                    continue
                i += 1
                continue
            if in_string:
                if char == "\\":
                    i += 2
                    continue
                if char == string_char:
                    in_string = False
                i += 1
                continue
            if char in ('"', "'"):
                in_string = True
                string_char = char
                i += 1
                continue
            if char == "/" and i + 1 < length and line[i + 1] == "/":
                break
            if char == "/" and i + 1 < length and line[i + 1] == "*":
                in_block = True
                line_masked = True
                i += 2
                continue
            i += 1
        mask.append(line_masked)
    return mask


def _can_join_line(line_n: str, line_next: str, n_in_block: bool, next_in_block: bool, max_length: int) -> bool:
    n = line_n.rstrip()
    m = line_next.strip()

    if not n or not m:
        return False
    if n_in_block or next_in_block:
        return False
    if n.lstrip().startswith("#") or m.startswith("#"):
        return False
    if _has_line_comment(n) or _has_line_comment(m):
        return False
    if len(n) + 1 + len(m) > max_length:
        return False

    ends = _ends_with_trigger(n)

    if m.startswith("{"):
        if ends:
            return True
        return bool(_CONTROL_HEADER.match(n))

    if m.startswith("}"):
        if not ends:
            return False
        if n.strip() == "{":
            return False
        return True

    if ends:
        if n.endswith("{"):
            if n.strip() == "{" or _OPEN_BRACE_HEADER.search(n) or _TYPE_HEADER.match(n):
                return False
        return True

    if not _starts_with_trigger(m):
        return False
    if _CONTROL_HEADER_LINE.match(n) and m[0] in _UNARY_START:
        return False
    return True


def _join_lines_pass(lines: List[str], max_length: int) -> List[str]:
    block_mask = _block_comment_mask(lines)
    result: List[str] = []
    i = 0
    total = len(lines)
    while i < total:
        if (
            i + 1 < total
            and _can_join_line(lines[i], lines[i + 1], block_mask[i], block_mask[i + 1], max_length)
        ):
            result.append(lines[i].rstrip() + " " + lines[i + 1].strip())
            i += 2
        else:
            result.append(lines[i])
            i += 1
    return result


def join_lines(code: str, max_length: int = 120) -> str:
    """Merge consecutive C++ lines that fit on a single line of max_length."""
    if not code:
        return code

    lines = code.splitlines()
    while True:
        merged_lines = _join_lines_pass(lines, max_length)
        if merged_lines == lines:
            break
        lines = merged_lines

    joined = "\n".join(lines)
    if code.endswith("\n"):
        joined += "\n"
    return joined


__all__ = ["join_lines"]