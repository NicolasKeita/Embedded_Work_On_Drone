#!/usr/bin/env python3
"""
Local Variable Alignment Formatter

Vertical alignment pass for C++ function bodies. In every function
definition the very first contiguous run of local variable declarations --
the block that starts right after the opening brace '{' -- is reformatted so
that the variable names all begin on the same column, computed from the
longest declaration type of the block plus one separator space. The
initializer that follows the name ('=' value, '{...}', '(args)') stays
attached to the name, so alignment only widens the gap between the type and
the variable name.

Scope rules
-----------
* Only the *first* contiguous declaration block at the top of a function body
  is aligned. A statement that opens a multi-line initializer ('Type name{',
  'Type name(' or 'Type name = make(') is collected as one logical
  declaration: its continuation lines are consumed verbatim until the brace
  depth returns to the statement base and the closing ';' is reached, so a
  wrapped initializer never splits the block in two.
* The first line that is not the start of a variable declaration (control
  structure, function call, reassignment, blank line, comment, ...) ends the
  block, and the rest of the function body is copied verbatim: later
  declaration blocks are left alone.
* Alignment is applied only when the block contains at least two
  declarations; a lone declaration keeps its original spacing.
* The alignment column is computed once from the longest type of the *whole*
  block -- template types like 'std::array<FaultScenario, 1>' included -- so
  every variable name of the block starts at indentation + max type width + 1,
  including the first line of multi-line statements.
* Leading blank lines right after the opening brace are skipped (and
  preserved) so the block can still be found and aligned.
* Base indentation and trailing initializers are preserved; trailing line
  comments are detached, the line is realigned, then the comment is
  re-attached after the rebuilt initializer.

The pass reuses the shared declaration parser (with its constructor-call
'allow_paren_init' mode, which is only legal at function scope) and the same
function-open-brace detection as the declaration blank-line formatter. It is
meant to run after line joining so wrapped declarations are already back on
one line; it is applied to non module-interface files, matching the other
local-declaration passes.
"""

from typing import List, NamedTuple, Optional, Tuple

from shared.brace_utils import brace_delta
from shared.declaration_parse import (
    DeclarationParts,
    mask_literals,
    parse_declaration,
    split_trailing_comment,
)
from formatter.declaration_blank_line_formatter import is_function_open_brace


class StatementBlock(NamedTuple):
    """One logical declaration statement of an alignment block.

    ``parts`` carries the indent / type / name parsed from the statement's
    first line, ``tail`` is the raw text that follows the name on that line
    ('{', ' = make(', ' = 1;', ...), ``continuation`` holds the verbatim
    continuation lines of a multi-line statement and ``original_line`` is the
    untouched first line, used when the block is emitted without alignment.
    """

    parts: DeclarationParts
    tail: str
    continuation: Tuple[str, ...]
    original_line: str


def _parse_local_declaration(line: str) -> Optional[DeclarationParts]:
    """Parse a single function-body line as a local variable declaration.

    Trailing line comments are split off first (and re-attached by the caller)
    and the constructor-call form 'Type name(args);' is accepted through the
    shared parser's ``allow_paren_init`` flag, which is only meaningful at
    function scope.
    """
    code_part, comment, _ = split_trailing_comment(line, False)
    if not code_part.strip().endswith(";"):
        return None
    return parse_declaration(code_part, comment, line, allow_paren_init=True)


def _parse_statement_start(line: str) -> Optional[Tuple[DeclarationParts, str]]:
    """Parse the first line of a multi-line declaration statement.

    The line must end with an opened brace or parenthesis initializer
    ('Type name{', 'Type name(' or 'Type name = make('). The declaration is
    parsed from a probe line where the opened initializer is temporarily
    closed, and the raw text following the variable name on the original line
    is returned alongside the parsed parts so the caller can rebuild the line
    with alignment while keeping the opened initializer verbatim.
    """
    code_part, comment, _ = split_trailing_comment(line, False)
    stripped = code_part.rstrip()
    if not stripped.endswith(("(", "{")):
        return None
    probe = stripped[:-1].rstrip() + ";"
    if probe == ";":
        return None
    parsed = parse_declaration(probe, comment, line, allow_paren_init=True)
    if parsed is None:
        return None
    masked_head = mask_literals(stripped[:-1])
    name_start = masked_head.find(parsed.name, len(parsed.indent) + len(parsed.type_part))
    if name_start < 0:
        return None
    tail = stripped[name_start + len(parsed.name):]
    return parsed, tail


def _collect_statement(
    lines: List[str],
    start: int,
    brace_depth: int,
) -> Tuple[Optional[StatementBlock], int, int]:
    """Collect one full statement (possibly spanning several lines) starting
    at ``lines[start]`` while the enclosing function body sits at
    ``brace_depth``.

    Returns ``(statement, next_index, new_depth)``. ``statement`` is None when
    the line is not the start of a variable declaration; ``next_index`` then
    points at the first unconsumed line (equal to ``start`` when nothing was
    consumed, or past a multi-line statement that could not be classified and
    must be copied verbatim). A statement ends on the line that brings the
    brace depth back to the statement base and ends with ';'.
    """
    n = len(lines)
    line = lines[start]
    stripped = line.strip()
    if not stripped:
        return None, start, brace_depth
    depth = brace_depth + brace_delta(line)
    if depth == brace_depth and stripped.endswith(";"):
        parsed = _parse_local_declaration(line)
        if parsed is None:
            return None, start, brace_depth
        return StatementBlock(parsed, parsed.rest, (), line), start + 1, depth
    statement_start = _parse_statement_start(line)
    if statement_start is None:
        return None, start, brace_depth
    parts, tail = statement_start
    continuation: List[str] = []
    i = start + 1
    while i < n:
        current = lines[i]
        depth += brace_delta(current)
        continuation.append(current)
        i += 1
        if depth <= 0:
            break
        if depth == brace_depth and current.strip().endswith(";"):
            return StatementBlock(parts, tail, tuple(continuation), line), i, depth
    return None, i, depth


def _flush_block(block: List[StatementBlock], result: List[str]) -> None:
    """Emit the collected first-block statements: aligned on one column when
    the block has at least two of them, otherwise verbatim and untouched."""
    if len(block) < 2:
        for statement in block:
            result.append(statement.original_line)
            result.extend(statement.continuation)
        block.clear()
        return
    target = max(len(statement.parts.type_part) for statement in block) + 1
    for statement in block:
        parts = statement.parts
        rebuilt = (
            parts.indent
            + parts.type_part
            + " " * (target - len(parts.type_part))
            + parts.name
            + statement.tail
        )
        if parts.comment:
            rebuilt = rebuilt.rstrip() + " " + parts.comment
        result.append(rebuilt)
        result.extend(statement.continuation)
    block.clear()


def _copy_until_close(lines: List[str], start: int, result: List[str], brace_depth: int) -> int:
    """Copy lines verbatim from ``start`` until the function's closing brace
    brings ``brace_depth`` back to zero. Returns the index past that brace."""
    n = len(lines)
    i = start
    while i < n:
        line = lines[i]
        brace_depth += brace_delta(line)
        result.append(line)
        if brace_depth <= 0:
            return i + 1
        i += 1
    return i


def _process_first_block(lines: List[str], start: int, result: List[str]) -> int:
    """Align the first contiguous declaration block of the function body whose
    first line is ``lines[start]``. Returns the index to resume scanning at."""
    n = len(lines)
    i = start
    brace_depth = 1
    block: List[StatementBlock] = []

    while i < n:
        line = lines[i]
        stripped = line.strip()
        delta = brace_delta(line)

        if brace_depth + delta <= 0:
            _flush_block(block, result)
            result.append(line)
            return i + 1

        if not block and not stripped:
            result.append(line)
            brace_depth += delta
            i += 1
            continue

        statement, next_index, new_depth = _collect_statement(lines, i, brace_depth)
        if statement is None and next_index == i:
            _flush_block(block, result)
            result.append(line)
            return _copy_until_close(lines, i + 1, result, brace_depth + delta)
        if statement is None:
            _flush_block(block, result)
            result.extend(lines[i:next_index])
            if new_depth <= 0:
                return next_index
            return _copy_until_close(lines, next_index, result, new_depth)

        block.append(statement)
        brace_depth = new_depth
        i = next_index

    _flush_block(block, result)
    return i


def align_first_declaration_blocks(code: str) -> str:
    """Align the first contiguous block of local variable declarations at the
    top of every C++ function body found in ``code``."""
    if not code:
        return code
    lines = code.splitlines()
    result: List[str] = []
    n = len(lines)
    i = 0
    while i < n:
        line = lines[i]
        if is_function_open_brace(line, lines, i) and brace_delta(line) > 0:
            result.append(line)
            i = _process_first_block(lines, i + 1, result)
            continue
        result.append(line)
        i += 1
    return "\n".join(result) + ("\n" if code.endswith("\n") else "")


__all__ = ["align_first_declaration_blocks"]
