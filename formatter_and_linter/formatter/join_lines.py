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

Multi-line function signatures (return type + name, e.g. a definition or a
prototype whose parameters are aligned by the parameter formatter) are left
untouched: the parameters are never joined back onto a single line. Access
specifiers ('public:', 'private:', 'protected:') and case/default/goto labels
that end a line with ':' never swallow the following statement. The same
applies when such a label already carries its opening brace on the same line
(e.g. 'case 1: {'). Finally, stream
statements (std::cout, std::cerr, ...) written across several lines with '<<'
or '>>' continuations are never joined.

Consecutive declarations ending with ';' or '}' are never merged together, and
a line starting with an attribute specifier ('[[nodiscard]]', ...) is never
joined onto the previous line. Constructor member initializer lists (the
continuation lines starting with ':' or ',' that follow a constructor
signature) are kept untouched as well.

Enum declarations (enum, enum class, enum struct) are never compacted: every
enumerator stays on its own line. A logical chain written with '&&' / '||'
across several lines is joined only when it holds a single operator (e.g.
'if (a\n    && b) {'); when it holds several operators each condition keeps its
own line. A statement wrapped over several lines (e.g. a std::array initialized
with '{{ ... }}' or a long call) is left completely untouched when the whole
statement, joined back onto one line, would exceed max_length.
"""

import re
from typing import List

from shared.brace_utils import CONTROL_KEYWORDS
from shared.function_analysis import has_stream_operators

_CONTROL_HEADER = re.compile(r"^\s*(?:if|else|for|while|switch|catch|do|try)\b")
_CONTROL_HEADER_LINE = re.compile(r"^\s*(?:if|else|for|while|switch|catch|do|try)\b(?:.*\))?\s*$")
_TYPE_HEADER = re.compile(r"^\s*(?:namespace|class|struct|union|enum)\b")
_OPEN_BRACE_HEADER = re.compile(r"(?:\)|\]|else|do|try)\s*\{$")
_ENUM_START = re.compile(r"^\s*enum\b")

_FUNC_DEF_START = re.compile(r"^\s*(?:(?:static|inline|virtual|explicit|constexpr|const)\s+)*[\w:<>]+(?:\s*[*&])*\s+([\w:<>]+)\s*\(")
_QUALIFIED_SIG_START = re.compile(r"^\s*[\w:<>,]+::~?[\w:]+\s*\(")
_ACCESS_SPECIFIER = re.compile(r"^\s*(?:public|private|protected)\s*:\s*$")
_CASE_LABEL = re.compile(r"^\s*(?:case\b.*|default)\s*:\s*\{?\s*$")
_LABEL = re.compile(r"^\s*[A-Za-z_]\w*\s*:\s*$")

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


def _signature_mask(lines: List[str]) -> List[bool]:
    """
    Return a mask marking every line belonging to a multi-line function
    signature (definition or prototype). Such lines are never joined, so the
    aligned parameters produced by the parameter formatter are preserved.
    """
    mask = [False] * len(lines)
    total = len(lines)
    i = 0
    while i < total:
        match = _FUNC_DEF_START.match(lines[i])
        name = match.group(1).split("::")[-1] if match else ""
        if match and name not in CONTROL_KEYWORDS:
            depth = lines[i].count("(") - lines[i].count(")")
            if depth > 0:
                while i < total and depth > 0:
                    mask[i] = True
                    i += 1
                    if i < total:
                        depth += lines[i].count("(") - lines[i].count(")")
                if i < total:
                    mask[i] = True
                continue
        if _QUALIFIED_SIG_START.match(lines[i]):
            depth = lines[i].count("(") - lines[i].count(")")
            if depth > 0:
                while i < total and depth > 0:
                    mask[i] = True
                    i += 1
                    if i < total:
                        depth += lines[i].count("(") - lines[i].count(")")
                if i < total:
                    mask[i] = True
                continue
        i += 1
    return mask


def _count_logical_operators(line: str) -> int:
    """Count '&&' and '||' operators outside string/char literals."""
    in_string = False
    string_char = ""
    count = 0
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
        if line[i:i + 2] in ("&&", "||"):
            count += 1
            i += 2
            continue
        i += 1
    return count


def _starts_logical_chain(line: str) -> bool:
    return line.lstrip().startswith(("&&", "||"))


def _ends_logical_chain(line: str) -> bool:
    return line.rstrip().endswith(("&&", "||"))


def _logical_chain_mask(lines: List[str]) -> List[bool]:
    """
    Mark every line belonging to a '&&' / '||' chain that holds more than one
    operator. A single operator may be joined back onto one line; a
    multi-condition chain keeps each condition on its own line.
    """
    mask = [False] * len(lines)
    counts = [_count_logical_operators(line) for line in lines]
    total = len(lines)
    i = 0
    while i < total:
        if not _starts_logical_chain(lines[i]) and not _ends_logical_chain(lines[i]):
            i += 1
            continue
        j = i
        chain_sum = 0
        while j < total and (
            j == i
            or _starts_logical_chain(lines[j])
            or _ends_logical_chain(lines[j - 1])
        ):
            chain_sum += counts[j]
            j += 1
        if chain_sum > 1:
            start = i
            while start > 0:
                above = lines[start - 1].rstrip()
                if not above:
                    break
                if _starts_logical_chain(lines[start]) or _ends_with_trigger(above):
                    start -= 1
                else:
                    break
            for k in range(start, j):
                mask[k] = True
        i = j
    return mask


def _enum_mask(lines: List[str]) -> List[bool]:
    """
    Mark every line of an enum declaration, from the 'enum' keyword up to the
    closing '};', so those lines are never joined.
    """
    mask = [False] * len(lines)
    total = len(lines)
    i = 0
    while i < total:
        if not _ENUM_START.match(lines[i]):
            i += 1
            continue
        j = i
        depth = 0
        seen_open = False
        while j < total:
            if not seen_open and j > i:
                if not lines[j].lstrip().startswith("{"):
                    break
            mask[j] = True
            opens = lines[j].count("{")
            closes = lines[j].count("}")
            if not seen_open:
                if opens:
                    seen_open = True
                    depth = opens - closes
            else:
                depth += opens - closes
            if seen_open and depth <= 0:
                break
            j += 1
        i = j + 1
    return mask


def _init_list_mask(lines: List[str]) -> List[bool]:
    """
    Mark every line of a constructor member initializer list (the lines
    starting with ':' or ',' that follow a constructor signature ending with
    ')'), so those lines are never joined together or onto the signature.
    """
    mask = [False] * len(lines)
    total = len(lines)
    i = 0
    while i < total:
        if not lines[i].lstrip().startswith(":"):
            i += 1
            continue
        prev = i - 1
        while prev >= 0 and not lines[prev].strip():
            prev -= 1
        if prev < 0 or not lines[prev].rstrip().endswith(")"):
            i += 1
            continue
        j = i
        while j < total:
            mask[j] = True
            if not lines[j].rstrip().endswith(","):
                break
            j += 1
        i = j + 1
    return mask


def _is_statement_continuation(line_a: str, line_b: str) -> bool:
    """Return True when two consecutive lines belong to the same wrapped statement."""
    a = line_a.rstrip()
    b = line_b.strip()
    if not a or not b:
        return False
    if a.lstrip().startswith("#") or b.startswith("#"):
        return False
    if _has_line_comment(a) or _has_line_comment(b):
        return False
    return _ends_with_trigger(a) or _starts_with_trigger(b)


def _oversized_statement_mask(
    lines: List[str],
    max_length: int,
    protected_base: List[bool],
    stream_mask: List[bool],
) -> List[bool]:
    """
    Mark every line of a multi-line statement whose fully-joined length exceeds
    max_length (e.g. a std::array '{{ ... }}' block or a long call). Such
    statements are left completely untouched: neither the individual lines nor
    partial prefixes are joined. Statements that fit within max_length when
    joined are left free so the usual per-line merging applies.
    """
    mask = [False] * len(lines)
    total = len(lines)
    i = 0
    while i < total:
        if protected_base[i] or stream_mask[i]:
            i += 1
            continue
        j = i
        while j + 1 < total and not protected_base[j + 1] and not stream_mask[j + 1]:
            if not _is_statement_continuation(lines[j], lines[j + 1]):
                break
            j += 1
        if j > i:
            joined = lines[i].rstrip()
            for k in range(i + 1, j + 1):
                joined += " " + lines[k].strip()
            if len(joined) > max_length:
                for k in range(i, j + 1):
                    mask[k] = True
        i = j + 1
    return mask


def _can_join_line(line_n: str, line_next: str, n_protected: bool, m_protected: bool, max_length: int) -> bool:
    n = line_n.rstrip()
    m = line_next.strip()

    if not n or not m:
        return False
    if n_protected or m_protected:
        return False
    if n.lstrip().startswith("#") or m.startswith("#"):
        return False
    if _has_line_comment(n) or _has_line_comment(m):
        return False
    if m.startswith("[["):
        return False
    if n.endswith((";", "}")):
        return False
    if has_stream_operators(n):
        return False
    if m.startswith("<<") or m.startswith(">>"):
        return False
    if _ACCESS_SPECIFIER.match(n) or _CASE_LABEL.match(n) or _LABEL.match(n):
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
    signature_mask = _signature_mask(lines)
    logical_mask = _logical_chain_mask(lines)
    enum_mask = _enum_mask(lines)
    init_list_mask = _init_list_mask(lines)
    protected_base_mask = [
        block_mask[i] or signature_mask[i] or logical_mask[i] or enum_mask[i] or init_list_mask[i]
        for i in range(len(lines))
    ]
    stream_mask = [has_stream_operators(line) for line in lines]
    oversized_mask = _oversized_statement_mask(lines, max_length, protected_base_mask, stream_mask)
    result: List[str] = []
    i = 0
    total = len(lines)
    while i < total:
        n_protected = protected_base_mask[i] or stream_mask[i] or oversized_mask[i]
        m_protected = (
            i + 1 < total
            and (protected_base_mask[i + 1] or stream_mask[i + 1] or oversized_mask[i + 1])
        )
        if (
            i + 1 < total
            and _can_join_line(lines[i], lines[i + 1], n_protected, m_protected, max_length)
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