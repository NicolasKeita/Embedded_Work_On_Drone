#!/usr/bin/env python3
"""
Inline Function Body Checks for .cppm Module Interfaces

Forbids multi-line function bodies inside C++20 module interface files
(.cppm): implementations longer than one effective line must live in a
separate .cpp file; only single-line getters/setters are tolerated.

The check is a small state-machine parser built on the Python standard
library only (no Tree-sitter, no external dependency):

1. Comments (// and /* ... */) are blanked out first, so they neither count
   as code lines nor disturb brace matching.
2. String literals, character literals and raw strings are blanked out, so
   braces inside them are ignored.
3. Every brace block is classified as namespace / type (struct, class,
   union, enum) / function / other, so struct or namespace scopes are never
   mistaken for function bodies.
4. Only non-empty lines strictly inside the function braces count as
   effective code lines; a warning is emitted above one effective line.

Accepted heuristic limitations (lightweight linter, no full C++ grammar):
- A digit-valued char literal such as u8'8' may be misread as a numeric
  digit separator.
- An opening brace glued to an identifier without whitespace right after a
  constructor initializer list (``Foo() : x_{0}{``) is treated as a braced
  initializer.
"""

import bisect
import re
from enum import Enum, auto
from pathlib import Path
from typing import List, NamedTuple, Tuple

MAX_CPPM_INLINE_FUNCTION_BODY_LINES = 1

WARN_CPPM_INLINE_FUNCTION_TAG = "[WARN_CPPM_INLINE_FUNCTION]"

_NAMESPACE_KEYWORD_PATTERN = re.compile(r'\bnamespace\b')
_TYPE_KEYWORD_PATTERN = re.compile(r'\b(?:struct|class|union|enum)\b')
_TYPE_DECLARATION_PATTERN = re.compile(r'\b(?:struct|class|union|enum)\s+\w+\s*$')
_ACCESS_LABEL_PATTERN = re.compile(r'\s*(?:public|private|protected)\s*:')
_LEADING_WHITESPACE_PATTERN = re.compile(r'\s*')
_FUNCTION_NAME_PATTERN = re.compile(r'(?:operator\s*[^\s(]*|(?:\w+::)*~?\w+)\s*$')
_WHITESPACE_RUN_PATTERN = re.compile(r'\s+')

_CONTROL_KEYWORDS = frozenset({
    'if', 'else', 'for', 'while', 'do', 'switch', 'case', 'catch', 'try',
    'return', 'throw', 'delete', 'new', 'sizeof', 'alignof', 'alignas',
    'typeid', 'decltype', 'noexcept', 'static_assert', 'requires', 'using',
    'typedef', 'static_cast', 'dynamic_cast', 'reinterpret_cast',
    'const_cast', 'concept', 'co_await', 'co_yield', 'co_return',
})


class _SanitizeState(Enum):
    CODE = auto()
    LINE_COMMENT = auto()
    BLOCK_COMMENT = auto()
    STRING = auto()
    CHAR = auto()
    RAW_STRING = auto()


class _BlockKind(Enum):
    NAMESPACE = auto()
    TYPE = auto()
    FUNCTION = auto()
    OTHER = auto()


class _Block(NamedTuple):
    kind: _BlockKind
    name: str
    open_pos: int
    start_pos: int
    transparent: bool


def _is_digit_separator(code: str, index: int) -> bool:
    if index <= 0 or index + 1 >= len(code):
        return False
    previous = code[index - 1]
    following = code[index + 1]
    if not (previous.isdigit() and (following.isalnum() or following == '_')):
        return False
    run_start = index - 1
    while run_start > 0 and code[run_start - 1].isdigit():
        run_start -= 1
    before_run = code[run_start - 1] if run_start > 0 else ''
    return not (before_run.isalpha() or before_run == '_')


def _raw_string_prefix_length(code: str, quote_index: int) -> int:
    if quote_index <= 0 or code[quote_index - 1] != 'R':
        return 0
    r_index = quote_index - 1
    before_r = r_index - 1
    if before_r < 0:
        return 1
    previous = code[before_r]
    if previous in ('u', 'U', 'L'):
        before_prefix = before_r - 1
        if before_prefix < 0 or not (code[before_prefix].isalnum() or code[before_prefix] == '_'):
            return 2
        return 0
    if previous == '8' and before_r - 1 >= 0 and code[before_r - 1] == 'u':
        before_prefix = before_r - 2
        if before_prefix < 0 or not (code[before_prefix].isalnum() or code[before_prefix] == '_'):
            return 3
        return 0
    if previous.isalnum() or previous == '_':
        return 0
    return 1


def _enter_literal(code: str, chars: List[str], quote_index: int) -> Tuple[int, _SanitizeState, str]:
    prefix_length = _raw_string_prefix_length(code, quote_index)
    if prefix_length == 0:
        chars[quote_index] = ' '
        return quote_index + 1, _SanitizeState.STRING, ''
    delimiter_end = code.find('(', quote_index + 1, quote_index + 18)
    delimiter = code[quote_index + 1:delimiter_end] if delimiter_end != -1 else ''
    if delimiter_end == -1 or any(char in ' \\()' for char in delimiter):
        chars[quote_index] = ' '
        return quote_index + 1, _SanitizeState.STRING, ''
    for offset in range(quote_index - prefix_length, delimiter_end + 1):
        chars[offset] = ' '
    terminator = ')' + delimiter + '"'
    return delimiter_end + 1, _SanitizeState.RAW_STRING, terminator



def _sanitize_code(code: str) -> str:
    chars = list(code)
    length = len(code)
    state = _SanitizeState.CODE
    raw_terminator = ''
    index = 0
    while index < length:
        char = code[index]
        following = code[index + 1] if index + 1 < length else ''
        if state == _SanitizeState.CODE:
            if char == '/' and following == '/':
                chars[index] = ' '
                chars[index + 1] = ' '
                index += 2
                state = _SanitizeState.LINE_COMMENT
            elif char == '/' and following == '*':
                chars[index] = ' '
                chars[index + 1] = ' '
                index += 2
                state = _SanitizeState.BLOCK_COMMENT
            elif char == '"':
                index, state, raw_terminator = _enter_literal(code, chars, index)
            elif char == "'":
                if _is_digit_separator(code, index):
                    index += 1
                else:
                    chars[index] = ' '
                    index += 1
                    state = _SanitizeState.CHAR
            else:
                index += 1
        elif state == _SanitizeState.LINE_COMMENT:
            if char == '\n':
                state = _SanitizeState.CODE
            else:
                chars[index] = ' '
            index += 1
        elif state == _SanitizeState.BLOCK_COMMENT:
            if char == '*' and following == '/':
                chars[index] = ' '
                chars[index + 1] = ' '
                index += 2
                state = _SanitizeState.CODE
            else:
                if char != '\n':
                    chars[index] = ' '
                index += 1
        elif state == _SanitizeState.STRING or state == _SanitizeState.CHAR:
            quote = '"' if state == _SanitizeState.STRING else "'"
            if char == '\\':
                chars[index] = ' '
                if following and following != '\n':
                    chars[index + 1] = ' '
                    index += 2
                else:
                    index += 1
            elif char == quote:
                chars[index] = ' '
                index += 1
                state = _SanitizeState.CODE
            elif char == '\n':
                state = _SanitizeState.CODE
                index += 1
            else:
                chars[index] = ' '
                index += 1
        elif state == _SanitizeState.RAW_STRING:
            if code.startswith(raw_terminator, index):
                for offset in range(index, index + len(raw_terminator)):
                    chars[offset] = ' '
                index += len(raw_terminator)
                state = _SanitizeState.CODE
            else:
                if char != '\n':
                    chars[index] = ' '
                index += 1
    return ''.join(chars)



def _compute_line_starts(code: str) -> List[int]:
    starts = [0]
    for match in re.finditer('\n', code):
        starts.append(match.end())
    return starts


def _line_of_position(line_starts: List[int], position: int) -> int:
    return bisect.bisect_right(line_starts, position) - 1


def _split_header(header: str) -> Tuple[str, int]:
    offset = 0
    while True:
        label_match = _ACCESS_LABEL_PATTERN.match(header, offset)
        if not label_match:
            break
        offset = label_match.end()
    whitespace_match = _LEADING_WHITESPACE_PATTERN.match(header, offset)
    offset = whitespace_match.end()
    return header[offset:], offset


def _find_parameter_list_paren(header: str) -> int:
    template_depth = 0
    for index, char in enumerate(header):
        if char == '<':
            template_depth += 1
        elif char == '>':
            template_depth = max(0, template_depth - 1)
        elif char == '(' and template_depth == 0:
            return index
    return -1


def _classify_header(header: str) -> Tuple[_BlockKind, str]:
    normalized = ' '.join(header.split())
    if not normalized or normalized.startswith('#'):
        return _BlockKind.OTHER, ''
    if _NAMESPACE_KEYWORD_PATTERN.search(normalized):
        return _BlockKind.NAMESPACE, ''
    if _TYPE_DECLARATION_PATTERN.search(normalized):
        return _BlockKind.TYPE, ''
    paren_index = _find_parameter_list_paren(normalized)
    if paren_index == -1:
        if _TYPE_KEYWORD_PATTERN.search(normalized):
            return _BlockKind.TYPE, ''
        return _BlockKind.OTHER, ''
    name_match = _FUNCTION_NAME_PATTERN.search(normalized[:paren_index])
    if not name_match:
        return _BlockKind.OTHER, ''
    name = _WHITESPACE_RUN_PATTERN.sub('', name_match.group(0))
    if name.split('::')[-1] in _CONTROL_KEYWORDS:
        return _BlockKind.OTHER, ''
    return _BlockKind.FUNCTION, name


def _is_braced_initializer(code: str, segment_start: int, brace_index: int) -> bool:
    if brace_index == 0:
        return False
    previous = code[brace_index - 1]
    if not (previous.isalnum() or previous == '_'):
        return False
    header = code[segment_start:brace_index]
    if _NAMESPACE_KEYWORD_PATTERN.search(header) or _TYPE_KEYWORD_PATTERN.search(header):
        return False
    last_close_paren = header.rfind(')')
    if last_close_paren != -1 and ':' not in header[last_close_paren + 1:]:
        return False
    return True



def _count_effective_lines(
    sanitized: str,
    line_starts: List[int],
    open_pos: int,
    close_pos: int
) -> int:
    open_line = _line_of_position(line_starts, open_pos)
    close_line = _line_of_position(line_starts, close_pos)
    effective_lines = 0
    for line_number in range(open_line, close_line + 1):
        line_start = line_starts[line_number]
        if line_number + 1 < len(line_starts):
            line_end = line_starts[line_number + 1] - 1
        else:
            line_end = len(sanitized)
        if line_number == open_line and line_number == close_line:
            text = sanitized[open_pos + 1:close_pos]
        elif line_number == open_line:
            text = sanitized[open_pos + 1:line_end]
        elif line_number == close_line:
            text = sanitized[line_start:close_pos]
        else:
            text = sanitized[line_start:line_end]
        stripped = text.strip()
        if not stripped or stripped == '{' or stripped == '}':
            continue
        effective_lines += 1
    return effective_lines


def _scan_sanitized_code(sanitized: str, max_body_lines: int) -> List[Tuple[str, int, int]]:
    line_starts = _compute_line_starts(sanitized)
    blocks: List[_Block] = []
    violations: List[Tuple[str, int, int]] = []
    segment_start = 0
    paren_depth = 0
    index = 0
    length = len(sanitized)
    while index < length:
        char = sanitized[index]
        if char == '(':
            paren_depth += 1
        elif char == ')':
            paren_depth = max(0, paren_depth - 1)
        elif char == '{' and paren_depth == 0:
            if _is_braced_initializer(sanitized, segment_start, index):
                blocks.append(_Block(_BlockKind.OTHER, '', index, index, True))
            else:
                header = sanitized[segment_start:index]
                significant_header, offset = _split_header(header)
                kind, name = _classify_header(significant_header)
                blocks.append(_Block(kind, name, index, segment_start + offset, False))
                segment_start = index + 1
        elif char == '}' and paren_depth == 0:
            if blocks:
                block = blocks.pop()
                if block.kind == _BlockKind.FUNCTION:
                    effective_lines = _count_effective_lines(
                        sanitized, line_starts, block.open_pos, index
                    )
                    if effective_lines > max_body_lines:
                        start_line = _line_of_position(line_starts, block.start_pos) + 1
                        violations.append((block.name, start_line, effective_lines))
                if not block.transparent:
                    segment_start = index + 1
            else:
                segment_start = index + 1
        elif char == ';' and paren_depth == 0:
            segment_start = index + 1
        index += 1
    violations.sort(key=lambda violation: violation[1])
    return violations


def check_cppm_inline_function_bodies(
    code: str,
    max_body_lines: int = MAX_CPPM_INLINE_FUNCTION_BODY_LINES
) -> List[Tuple[str, int, int]]:
    sanitized = _sanitize_code(code)
    return _scan_sanitized_code(sanitized, max_body_lines)


def format_cppm_inline_function_message(
    file_path: str,
    function_name: str,
    start_line: int,
    body_lines: int
) -> str:
    return (
        f"{WARN_CPPM_INLINE_FUNCTION_TAG} {file_path}:{start_line} : "
        f"la fonction '{function_name}' possède un corps de {body_lines} lignes "
        f"effectives dans l'interface de module "
        f"(max : {MAX_CPPM_INLINE_FUNCTION_BODY_LINES}). "
        "Déplace l'implémentation dans un fichier .cpp."
    )


def check_cppm_inline_functions(file_path: Path) -> list[str]:
    if file_path.suffix != '.cppm':
        return []
    code = file_path.read_text(encoding='utf-8')
    violations = check_cppm_inline_function_bodies(code)
    return [
        format_cppm_inline_function_message(str(file_path), function_name, start_line, body_lines)
        for function_name, start_line, body_lines in violations
    ]

