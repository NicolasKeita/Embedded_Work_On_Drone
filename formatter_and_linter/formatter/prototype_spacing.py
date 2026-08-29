#!/usr/bin/env python3
"""
Prototype Spacing & Character Cleaning

Standalone cleaning pass for C++ source files. Removes non-breaking
spaces, strips trailing whitespace and collapses blank lines between
consecutive function prototypes while preserving the blank line that
aerates the start of a real function definition and the blank lines
between logical blocks (using declarations, imports, ...).

The collapsing only happens at namespace / top-level scope: blank lines
inside function bodies or class definitions are left untouched.

This module only relies on the Python standard library and can also be
run directly as a script:

    python formatter/prototype_spacing.py file.cppm [file2.cpp ...]
    python formatter/prototype_spacing.py -i file.cppm
"""

import re
import sys
from typing import List

NON_BREAKING_SPACE_CHARS = ("\xa0", "\u202f", "\u2007")

NAMESPACE_OPEN_REGEX = re.compile(r"(?:inline\s+|export\s+)*namespace(?:\s+[\w:<>]+)?\s*$")

NON_PROTOTYPE_REGEX = re.compile(r"^\s*(using|typedef|import|module|#|return|case|goto|else|do)\b")

CONTROL_STATEMENT_REGEX = re.compile(r"^\s*(static_assert|noexcept|throw)\s*\(")

DELETE_DEFAULT_REGEX = re.compile(r"\)\s*=\s*(delete|default)\s*;")

FUNCTION_CALL_REGEX = re.compile(r"\w\s*\(")


def _normalize_characters(lines: List[str]) -> List[str]:
    cleaned = []
    for line in lines:
        for char in NON_BREAKING_SPACE_CHARS:
            line = line.replace(char, " ")
        cleaned.append(line.rstrip())
    return cleaned


def _strip_comments(lines: List[str]) -> List[str]:
    code_lines = []
    in_block_comment = False

    for line in lines:
        result = []
        index = 0
        length = len(line)

        while index < length:
            if in_block_comment:
                end = line.find("*/", index)
                if end == -1:
                    index = length
                else:
                    in_block_comment = False
                    index = end + 2
                continue

            if line.startswith("//", index):
                break

            if line.startswith("/*", index):
                in_block_comment = True
                index += 2
                continue

            char = line[index]

            if char == '"':
                result.append('"')
                index += 1
                while index < length:
                    if line[index] == "\\" and index + 1 < length:
                        index += 2
                        continue
                    if line[index] == '"':
                        index += 1
                        break
                    index += 1
                result.append('"')
                continue

            if char == "'":
                result.append("'")
                index += 1
                while index < length:
                    if line[index] == "\\" and index + 1 < length:
                        index += 2
                        continue
                    if line[index] == "'":
                        index += 1
                        break
                    index += 1
                result.append("'")
                continue

            result.append(char)
            index += 1

        code_lines.append("".join(result))

    return code_lines


def _paren_balance(code: str) -> int:
    return code.count("(") - code.count(")")



def _is_prototype_candidate(code: str) -> bool:
    stripped = code.strip()

    if not stripped or stripped.startswith("#"):
        return False
    if "{" in stripped or "}" in stripped:
        return False
    if "(" not in stripped:
        return False
    if NON_PROTOTYPE_REGEX.match(stripped):
        return False
    if CONTROL_STATEMENT_REGEX.match(stripped):
        return False
    if DELETE_DEFAULT_REGEX.search(stripped):
        return False
    if not FUNCTION_CALL_REGEX.search(stripped):
        return False

    return True


def _looks_like_prototype(code: str) -> bool:
    stripped = code.strip()

    if not stripped.endswith(";"):
        return False
    if ")" not in stripped:
        return False

    return _is_prototype_candidate(code)


def _looks_like_prototype_start(code: str) -> bool:
    stripped = code.strip()

    if stripped.endswith(";"):
        return False

    return _is_prototype_candidate(code)


def _apply_braces(code: str, scope_stack: List[str]) -> None:
    prefix_chars = []

    for char in code:
        if char == "{":
            prefix = "".join(prefix_chars).strip()
            if NAMESPACE_OPEN_REGEX.search(prefix):
                scope_stack.append("namespace")
            else:
                scope_stack.append("other")
            prefix_chars = []
        elif char == "}":
            prefix_chars = []
            if scope_stack:
                scope_stack.pop()
        else:
            prefix_chars.append(char)


def _multiline_signature_ends_as_prototype(stripped: List[str], start: int) -> bool:
    balance = 0

    for index in range(start, len(stripped)):
        code = stripped[index]

        if index > start and code.strip() == "":
            continue

        balance += _paren_balance(code)

        if "{" in code:
            return False

        if balance <= 0:
            return code.strip().endswith(";")

    return False


def collapse_prototype_blank_lines(lines: List[str]) -> List[str]:
    stripped = _strip_comments(lines)
    output: List[str] = []
    scope_stack: List[str] = []
    pending_blanks = 0
    in_multiline_prototype = False
    multiline_balance = 0
    prev_ends_prototype = False

    for index, raw in enumerate(lines):
        if raw.strip() == "":
            pending_blanks += 1
            continue

        code = stripped[index]
        at_namespace_level = all(scope == "namespace" for scope in scope_stack)
        ends_semi = code.rstrip().endswith(";")
        has_brace = "{" in code or "}" in code

        if in_multiline_prototype:
            multiline_balance += _paren_balance(code)
            flush_as_prototype = True
            ends_prototype = True
            if has_brace:
                in_multiline_prototype = False
                flush_as_prototype = False
            elif multiline_balance <= 0 and ends_semi:
                in_multiline_prototype = False
        else:
            balance = _paren_balance(code)

            if balance == 0 and ends_semi and _looks_like_prototype(code):
                flush_as_prototype = True
                ends_prototype = True
            elif balance > 0 and not ends_semi and not has_brace and _looks_like_prototype_start(code):
                in_multiline_prototype = True
                multiline_balance = balance
                flush_as_prototype = _multiline_signature_ends_as_prototype(stripped, index)
                ends_prototype = True
            else:
                flush_as_prototype = False
                ends_prototype = False

        if pending_blanks and output:
            if at_namespace_level:
                if not (prev_ends_prototype and flush_as_prototype):
                    output.append("")
            else:
                output.extend([""] * pending_blanks)
        pending_blanks = 0

        output.append(raw)
        prev_ends_prototype = ends_prototype

        _apply_braces(code, scope_stack)

    return output


def clean_code(code: str) -> str:
    had_trailing_newline = code.endswith("\n")
    lines = code.splitlines()
    lines = _normalize_characters(lines)
    lines = collapse_prototype_blank_lines(lines)
    cleaned = "\n".join(lines)
    if cleaned and had_trailing_newline:
        cleaned += "\n"
    return cleaned


def main() -> None:
    arguments = sys.argv[1:]
    in_place = "-i" in arguments
    file_paths = [argument for argument in arguments if argument != "-i"]

    if not file_paths:
        print("Usage: python prototype_spacing.py [-i] <file.cpp|file.cppm> ...", file=sys.stderr)
        print("  -i, --in-place : Modify the files in place (otherwise print to stdout)", file=sys.stderr)
        sys.exit(1)

    for file_path in file_paths:
        with open(file_path, "r", encoding="utf-8") as handle:
            content = handle.read()

        cleaned = clean_code(content)

        if in_place:
            with open(file_path, "w", encoding="utf-8", newline="") as handle:
                handle.write(cleaned)
                if not cleaned.endswith("\n"):
                    handle.write("\n")
        else:
            print(cleaned)


if __name__ == "__main__":
    main()
