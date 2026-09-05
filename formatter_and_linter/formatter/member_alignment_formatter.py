#!/usr/bin/env python3
"""
Member Variable Alignment Formatter

Vertical alignment pass for C++20 module interface files. Inside every
struct / class body, contiguous runs of member variable declarations get
their variable names aligned on a single column. The target column is
computed from the longest declaration type of the contiguous block plus one
separator space, so the initializer ('=' or '{') that follows the name stays
attached to the name instead of being pushed to its own column.

A block is broken (and the alignment recomputed) by blank lines, methods,
macros / preprocessor directives, visibility changes ('public:', 'private:',
'protected:'), closing braces or any other non-variable statement. Pure
comment lines are neutral: they neither join nor break a block. Trailing
line comments are detached before reformatting and re-attached after the
final semicolon of the rebuilt line, separated by a single space.

Complex types are supported: namespaced names ('a::b::C'), templates
('<...>', nested and containing commas or integer literals), fixed-width
types ('std::uint64_t'), qualifiers ('const', 'static', 'constexpr',
'mutable', 'unsigned', ...), pointers and references, array suffixes
('[3]', '[N][M]') and leading attributes ('[[no_unique_address]]').
Declarations that cannot be parsed with certainty (bit-fields, function
pointers, multiple declarators, elaborated type specifiers, ...) are left
untouched and simply break the surrounding block.

The pass only processes module interface files: every other extension
(.cpp, .hpp, .h, ...) is returned untouched.
"""

import re
from typing import List, NamedTuple, Optional

from shared.declaration_parse import (
    DeclarationParts,
    mask_literals,
    parse_declaration,
    split_trailing_comment,
)

CPPM_EXTENSION = ".cppm"

MAX_LINE_LENGTH = 120

_CLASS_HEADER_REGEX = re.compile(
    r"^\s*(?:export\s+)?(?:template\s*<[^<>]*>\s*)?(?:struct|class|union)\b"
)

_PREPROCESSOR_REGEX = re.compile(r"^\s*#")


class _BlockItem(NamedTuple):
    """One buffered block entry: a parsed declaration or a neutral line
    (comment) kept in source order until the block is emitted."""

    declaration: Optional[DeclarationParts]
    raw_line: str


class _ScanState:
    """Brace-depth tracker that knows which scopes are struct/class bodies.

    'depth' counts the braces currently open. 'class_stack' stores, for each
    open struct / class / union body, the depth value measured right after
    its opening brace: a line is a member declaration candidate when it is
    processed while 'depth' equals that stored value. 'pending_header_depth'
    remembers a class header seen on a previous line whose opening brace has
    not been consumed yet (Allman brace style).
    """

    def __init__(self) -> None:
        self.depth = 0
        self.class_stack: List[int] = []
        self.pending_header_depth: Optional[int] = None
        self.in_block_comment = False

    def in_class_body(self) -> bool:
        return bool(self.class_stack) and self.depth == self.class_stack[-1]

    def update(self, masked_code: str) -> None:
        """Consume one code line (literals masked) and update the scope state."""
        has_header = _CLASS_HEADER_REGEX.match(masked_code) is not None
        if has_header:
            self.pending_header_depth = self.depth
        header_consumed = False
        for char in masked_code:
            if char == "{":
                can_open_class = (
                    self.pending_header_depth is not None
                    and not header_consumed
                    and self.depth == self.pending_header_depth
                    and (has_header or masked_code.strip() == "{")
                )
                if can_open_class:
                    self.depth += 1
                    self.class_stack.append(self.depth)
                    self.pending_header_depth = None
                    header_consumed = True
                else:
                    self.depth += 1
            elif char == "}":
                self.depth = max(0, self.depth - 1)
                while self.class_stack and self.depth < self.class_stack[-1]:
                    self.class_stack.pop()
                self.pending_header_depth = None
        if self.pending_header_depth is not None and not header_consumed:
            if (
                "{" in masked_code
                or "}" in masked_code
                or masked_code.rstrip().endswith(";")
            ):
                self.pending_header_depth = None


def _flush_block(block: List[_BlockItem], output: List[str], max_line_length: int) -> None:
    """Emit the pending block in source order, aligning the member names when
    every rebuilt line stays within max_line_length, or verbatim otherwise."""
    if not block:
        return
    declarations = [item.declaration for item in block if item.declaration is not None]
    keep_original = not declarations
    aligned_lines: List[str] = []
    if declarations:
        target_column = max(len(declaration.type_part) for declaration in declarations) + 1
        for declaration in declarations:
            line = (
                declaration.indent
                + declaration.type_part
                + " " * (target_column - len(declaration.type_part))
                + declaration.name
                + declaration.rest
            )
            if declaration.comment:
                line = line.rstrip() + " " + declaration.comment
            if len(line.rstrip()) > max_line_length:
                keep_original = True
                break
            aligned_lines.append(line)
    if keep_original:
        output.extend(item.raw_line for item in block)
    else:
        aligned_iter = iter(aligned_lines)
        for item in block:
            output.append(next(aligned_iter) if item.declaration is not None else item.raw_line)
    block.clear()


def align_member_variables(code: str, max_line_length: int = MAX_LINE_LENGTH) -> str:
    """Align member variable names inside every struct / class body of the code."""
    if not code:
        return code
    lines = code.splitlines()
    state = _ScanState()
    output: List[str] = []
    block: List[_BlockItem] = []

    for line in lines:
        was_in_block_comment = state.in_block_comment
        code_part, comment_part, block_comment_state = split_trailing_comment(
            line, state.in_block_comment
        )
        state.in_block_comment = block_comment_state
        stripped_code = code_part.strip()

        if was_in_block_comment or (not stripped_code and (comment_part or block_comment_state)):
            if stripped_code:
                state.update(mask_literals(stripped_code))
            block.append(_BlockItem(declaration=None, raw_line=line))
            continue

        if not stripped_code:
            _flush_block(block, output, max_line_length)
            output.append(line)
            continue

        if _PREPROCESSOR_REGEX.match(stripped_code):
            _flush_block(block, output, max_line_length)
            output.append(line)
            continue

        parsed = None
        if state.in_class_body() and stripped_code.endswith(";"):
            parsed = parse_declaration(code_part, comment_part, line)

        if parsed is None:
            _flush_block(block, output, max_line_length)
            output.append(line)
        else:
            block.append(_BlockItem(declaration=parsed, raw_line=line))

        state.update(mask_literals(stripped_code))

    _flush_block(block, output, max_line_length)

    aligned = "\n".join(output)
    if code.endswith("\n"):
        aligned += "\n"
    return aligned


def format_member_alignment_for_file(
    file_path: str,
    code: str,
    max_line_length: int = MAX_LINE_LENGTH,
) -> str:
    """Apply the alignment pass only when file_path is a .cppm module interface.

    Every other extension (.cpp, .hpp, .h, ...) is returned untouched: the
    rule is exclusive to C++20 module interface files.
    """
    if not file_path.lower().endswith(CPPM_EXTENSION):
        return code
    return align_member_variables(code, max_line_length)


__all__ = ["align_member_variables", "format_member_alignment_for_file"]
