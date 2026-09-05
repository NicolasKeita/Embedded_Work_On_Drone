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
* Only the *first* contiguous single-line declaration block at the top of a
  function body is aligned. The first line that is not a single-line variable
  declaration (control structure, function call, reassignment, blank line,
  comment, multi-line declaration, ...) ends the block, and the rest of the
  function body is copied verbatim: later declaration blocks are left alone.
* Alignment is applied only when the block contains at least two
  declarations; a lone declaration keeps its original spacing.
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

from typing import List, Optional

from shared.brace_utils import brace_delta
from shared.declaration_parse import (
    DeclarationParts,
    parse_declaration,
    split_trailing_comment,
)
from formatter.declaration_blank_line_formatter import is_function_open_brace


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


def _flush_block(block: List[DeclarationParts], result: List[str]) -> None:
    """Emit the collected first-block declarations: aligned on one column when
    the block has at least two of them, otherwise verbatim and untouched."""
    if len(block) < 2:
        for declaration in block:
            result.append(declaration.original_line)
        block.clear()
        return
    target = max(len(declaration.type_part) for declaration in block) + 1
    for declaration in block:
        rebuilt = (
            declaration.indent
            + declaration.type_part
            + " " * (target - len(declaration.type_part))
            + declaration.name
            + declaration.rest
        )
        if declaration.comment:
            rebuilt = rebuilt.rstrip() + " " + declaration.comment
        result.append(rebuilt)
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
    block: List[DeclarationParts] = []

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

        parsed = _parse_local_declaration(line)
        if parsed is None:
            _flush_block(block, result)
            result.append(line)
            brace_depth += delta
            return _copy_until_close(lines, i + 1, result, brace_depth)

        block.append(parsed)
        brace_depth += delta
        i += 1

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
