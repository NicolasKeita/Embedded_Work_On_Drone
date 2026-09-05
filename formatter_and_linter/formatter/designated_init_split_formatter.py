#!/usr/bin/env python3
"""
Designated Initializer Split Formatter

A formatting pass that splits single-line C++ declarations using a braced
initializer with C++20 designated initializers ('.field = value') over
several lines whenever the line exceeds the maximum line length (120
characters by default, indentation included).

The declaration and its opening brace stay on the original line, every
designated field is moved to its own line and the closing '};' is placed on
its own line. The field indentation is the column of the variable name plus
a constant offset (4 spaces) and the closing brace is aligned with the
variable name column, so the vertical alignment of the types and variable
names of the surrounding declarations (the 'table' alignment) is preserved.

Only top-level commas are used as split points: commas nested inside
function calls, template argument lists ('<...>') or nested braced
sub-initializers never break a sub-expression. A trailing comma is always
added after the last field and an optional trailing line comment is kept on
the closing brace line.

Lines that already fit within the maximum length, statements without a
designated initializer (plain braced lists, enumerators, class / struct /
namespace bodies, lambdas, call arguments) and statements that are already
split are left untouched, which makes the pass idempotent.
"""

import re
from typing import List, Optional, Tuple

MAX_LINE_LENGTH = 120
FIELD_INDENT = 4

_DESIGNATED_FIELD = re.compile(r"^\s*\.[A-Za-z_]\w*\s*(?:=|\{)")
_TYPE_KEYWORDS = frozenset({"struct", "class", "enum", "union", "namespace"})


def _semicolon_after(line: str, start: int) -> int:
    """
    Return the index of the first character of ``line`` at or after ``start``
    when it is a semicolon, -1 otherwise.
    """
    index = start
    while index < len(line) and line[index] in " \t":
        index += 1
    if index < len(line) and line[index] == ";":
        return index
    return -1


def _name_column_before(line: str, open_index: int) -> int:
    """
    Return the column of the identifier written just before the opening brace
    at ``open_index`` (optional spaces allowed between both), -1 when there is
    no such identifier (lambda, call argument, template closing bracket, ...)
    or when the identifier names a type being defined (struct / class / enum /
    union / namespace).
    """
    end = open_index
    while end > 0 and line[end - 1] in " \t":
        end -= 1
    start = end
    while start > 0 and (line[start - 1].isalnum() or line[start - 1] == "_"):
        start -= 1
    if start == end:
        return -1
    previous = line[start - 1] if start > 0 else ""
    if previous == "" or previous in ".:>":
        return -1
    word_start = start
    while word_start > 0 and (line[word_start - 1].isalnum() or line[word_start - 1] == "_"):
        word_start -= 1
    if line[word_start:start] in _TYPE_KEYWORDS:
        return -1
    return start


def _find_initializer(line: str) -> Optional[Tuple[int, int, int, int]]:
    """
    Locate the outermost braced initializer of a single-line statement.

    Returns (open_index, close_index, name_column, semicolon_index) when the
    line has the shape '<prefix><name>{...};' (a trailing line comment after
    the semicolon is allowed), otherwise None.
    """
    stack: List[Tuple[str, int]] = []
    last_brace_pair: Optional[Tuple[int, int]] = None
    in_string = False
    string_char = ""
    in_line_comment = False
    i = 0
    length = len(line)
    while i < length:
        char = line[i]
        if in_line_comment:
            break
        if in_string:
            if char == "\\":
                i += 2
                continue
            if char == string_char:
                in_string = False
            i += 1
            continue
        if char == "/" and i + 1 < length and line[i + 1] == "/":
            in_line_comment = True
            i += 2
            continue
        if char in ('"', "'"):
            in_string = True
            string_char = char
            i += 1
            continue
        if char in "([{":
            stack.append((char, i))
        elif char in ")]}":
            if not stack:
                return None
            open_char, open_index = stack.pop()
            if open_char == "{" and not stack:
                last_brace_pair = (open_index, i)
        i += 1
    if in_string or stack or last_brace_pair is None:
        return None
    open_index, close_index = last_brace_pair
    semicolon_index = _semicolon_after(line, close_index + 1)
    if semicolon_index == -1:
        return None
    name_column = _name_column_before(line, open_index)
    if name_column == -1:
        return None
    return (open_index, close_index, name_column, semicolon_index)


def _split_top_level_commas(text: str) -> List[str]:
    """
    Split ``text`` on its top-level commas only; commas nested inside
    parentheses, brackets, braces or angle brackets are preserved.
    """
    segments: List[str] = []
    current: List[str] = []
    depth = 0
    in_string = False
    string_char = ""
    i = 0
    length = len(text)
    while i < length:
        char = text[i]
        if in_string:
            current.append(char)
            if char == "\\" and i + 1 < length:
                current.append(text[i + 1])
                i += 2
                continue
            if char == string_char:
                in_string = False
            i += 1
            continue
        if char in ('"', "'"):
            in_string = True
            string_char = char
            current.append(char)
            i += 1
            continue
        if char in "([{<":
            depth += 1
            current.append(char)
            i += 1
            continue
        if char in ")]}>":
            depth = max(0, depth - 1)
            current.append(char)
            i += 1
            continue
        if char == "," and depth == 0:
            segments.append("".join(current))
            current = []
            i += 1
            continue
        current.append(char)
        i += 1
    segments.append("".join(current))
    return segments


def _has_designated_initializer(segments: List[str]) -> bool:
    """Tell whether any top-level segment starts with a designated field."""
    return any(_DESIGNATED_FIELD.match(segment) for segment in segments)


def _split_statement_line(line: str) -> List[str]:
    """
    Split one over-long designated-initializer declaration line into several
    lines, or return the line unchanged when it is not splittable.
    """
    found = _find_initializer(line)
    if found is None:
        return [line]
    open_index, close_index, name_column, semicolon_index = found
    segments = _split_top_level_commas(line[open_index + 1:close_index])
    if not _has_designated_initializer(segments):
        return [line]
    field_indent = " " * (name_column + FIELD_INDENT)
    closing_indent = " " * name_column
    trailing = line[semicolon_index + 1:]
    lines = [line[:open_index + 1]]
    for segment in segments:
        stripped = segment.strip()
        if stripped:
            lines.append(field_indent + stripped + ",")
    closing = closing_indent + "};"
    if trailing.strip():
        closing += trailing
    lines.append(closing)
    return lines


def split_long_designated_initializations(code: str, max_length: int = MAX_LINE_LENGTH) -> str:
    """
    Split declarations using a designated initializer that exceed max_length
    over several lines, one field per line.
    """
    if not code:
        return code
    lines = code.split("\n")
    result: List[str] = []
    for line in lines:
        if len(line.rstrip()) > max_length:
            result.extend(_split_statement_line(line))
        else:
            result.append(line)
    return "\n".join(result)


__all__ = ["split_long_designated_initializations", "MAX_LINE_LENGTH", "FIELD_INDENT"]

