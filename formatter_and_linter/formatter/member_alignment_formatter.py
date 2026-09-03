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
from typing import List, NamedTuple, Optional, Tuple

CPPM_EXTENSION = ".cppm"

MAX_LINE_LENGTH = 120

_CLASS_HEADER_REGEX = re.compile(
    r"^\s*(?:export\s+)?(?:template\s*<[^<>]*>\s*)?(?:struct|class|union)\b"
)

_PREPROCESSOR_REGEX = re.compile(r"^\s*#")

_IDENTIFIER_REGEX = re.compile(r"[A-Za-z_]\w*")

_IDENTIFIER_TAIL_REGEX = re.compile(r"[A-Za-z_]\w*$")

_ARRAY_SUFFIX_REGEX = re.compile(r"(?:\s*\[[^\[\]]*\])+\s*$")

_TYPE_TOKEN_REGEX = re.compile(r"[A-Za-z_]\w*|::|[<>*&\[\]]|\S")

_INTEGER_LITERAL_REGEX = re.compile(r"-?\d+")

_SPACE_BEFORE_SEMICOLON_REGEX = re.compile(r"\s+;$")

_NON_TYPE_KEYWORDS = frozenset([
    "alignas", "alignof", "break", "case", "catch", "class", "co_await",
    "co_return", "co_yield", "compl", "concept", "const_cast", "continue",
    "decltype", "default", "delete", "do", "dynamic_cast", "else", "enum",
    "export", "false", "for", "friend", "goto", "if", "import", "module",
    "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator",
    "or", "or_eq", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "sizeof", "static_assert",
    "static_cast", "struct", "switch", "template", "this", "throw", "true",
    "try", "typedef", "typeid", "union", "using", "virtual", "while",
    "xor", "xor_eq",
])

_RESERVED_NAMES = frozenset([
    "alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor",
    "bool", "break", "case", "catch", "char", "char8_t", "char16_t",
    "char32_t", "class", "compl", "concept", "const", "consteval",
    "constexpr", "constinit", "const_cast", "continue", "co_await",
    "co_return", "co_yield", "decltype", "default", "delete", "do", "double",
    "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false",
    "float", "for", "friend", "goto", "if", "inline", "int", "long",
    "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr",
    "operator", "or", "or_eq", "private", "protected", "public", "register",
    "reinterpret_cast", "requires", "return", "short", "signed", "sizeof",
    "static", "static_assert", "static_cast", "struct", "switch", "template",
    "this", "thread_local", "throw", "true", "try", "typedef", "typeid",
    "typename", "union", "unsigned", "using", "virtual", "void", "volatile",
    "wchar_t", "while", "xor", "xor_eq",
])


class _MemberDeclaration(NamedTuple):
    """One parsed member variable declaration line ready to be realigned."""

    indent: str
    type_part: str
    name: str
    rest: str
    comment: str
    original_line: str


class _BlockItem(NamedTuple):
    """One buffered block entry: a parsed declaration or a neutral line
    (comment) kept in source order until the block is emitted."""

    declaration: Optional[_MemberDeclaration]
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


def _mask_literals(line: str) -> str:
    """Replace string / char literals and inline block comments by spaces.

    The masked text keeps the exact length of the input, so indexes computed
    on it stay valid on the original text while braces, separators and
    parentheses hidden inside literals no longer disturb the scans.
    """
    chars = list(line)
    index = 0
    length = len(line)
    while index < length:
        char = line[index]
        if char in ('"', "'"):
            quote = char
            chars[index] = " "
            index += 1
            while index < length:
                if line[index] == "\\":
                    chars[index] = " "
                    if index + 1 < length:
                        chars[index + 1] = " "
                    index += 2
                    continue
                if line[index] == quote:
                    chars[index] = " "
                    index += 1
                    break
                chars[index] = " "
                index += 1
            continue
        if line.startswith("/*", index):
            end = line.find("*/", index + 2)
            if end == -1:
                for position in range(index, length):
                    chars[position] = " "
                index = length
            else:
                for position in range(index, end + 2):
                    chars[position] = " "
                index = end + 2
            continue
        index += 1
    return "".join(chars)


def _split_trailing_comment(line: str, in_block_comment: bool) -> Tuple[str, str, bool]:
    """Split a raw line into (code, trailing comment, new block-comment state).

    Line comments ('// ...') end the code part. Block comments are kept
    verbatim inside the code part when they close on the same line; an
    unterminated block comment makes the rest of the line the comment part
    and toggles the returned state so the following lines stay neutral.
    """
    index = 0
    length = len(line)
    code_end = 0
    while index < length:
        if in_block_comment:
            end = line.find("*/", index)
            if end == -1:
                return "", line, True
            in_block_comment = False
            index = end + 2
            code_end = index
            continue
        char = line[index]
        if char == '"':
            index += 1
            while index < length:
                if line[index] == "\\":
                    index += 2
                    continue
                if line[index] == '"':
                    index += 1
                    break
                index += 1
            code_end = index
            continue
        if char == "'":
            index += 1
            while index < length:
                if line[index] == "\\":
                    index += 2
                    continue
                if line[index] == "'":
                    index += 1
                    break
                index += 1
            code_end = index
            continue
        if line.startswith("//", index):
            return line[:code_end], line[index:], in_block_comment
        if line.startswith("/*", index):
            end = line.find("*/", index + 2)
            if end == -1:
                return line[:code_end], line[index:], True
            index = end + 2
            code_end = index
            continue
        index += 1
        code_end = index
    return line[:code_end], "", in_block_comment


def _find_declaration_split(body: str) -> Optional[int]:
    """Locate the first top-level '=' or '{' separating 'type name' from its
    initializer.

    Returns the index of the separator, -1 when the declaration carries no
    initializer, or None when the line cannot be a single variable
    declaration (parentheses, commas, extra semicolons at top level, i.e.
    methods, multiple declarators or function pointers).
    """
    angle_depth = 0
    for index, char in enumerate(body):
        if char == "<":
            angle_depth += 1
        elif char == ">":
            if angle_depth > 0:
                angle_depth -= 1
        elif angle_depth == 0:
            if char in ("(", ")"):
                return None
            if char in (",", ";"):
                return None
            if char in ("=", "{"):
                return index
    return -1


def _is_valid_type_part(type_part: str) -> bool:
    """Check that the text before the variable name looks like a C++ type."""
    if not type_part:
        return False
    tokens = _TYPE_TOKEN_REGEX.findall(type_part)
    if not tokens:
        return False
    if tokens[0] in _NON_TYPE_KEYWORDS:
        return False
    angle_depth = 0
    for token in tokens:
        if token == "<":
            angle_depth += 1
            continue
        if token == ">":
            if angle_depth == 0:
                return False
            angle_depth -= 1
            continue
        if token in ("::", "*", "&", "[", "]"):
            continue
        if token in ("(", ")"):
            if angle_depth == 0:
                return False
            continue
        if token == ",":
            if angle_depth == 0:
                return False
            continue
        if _IDENTIFIER_REGEX.fullmatch(token):
            continue
        if angle_depth > 0 and _INTEGER_LITERAL_REGEX.fullmatch(token):
            continue
        return False
    return angle_depth == 0


def _has_top_level_comma(text: str) -> bool:
    """Detect a comma separating several declarators inside an initializer."""
    depth = 0
    for char in text:
        if char in ("<", "(", "[", "{"):
            depth += 1
        elif char in (">", ")", "]", "}"):
            depth = max(0, depth - 1)
        elif char == "," and depth == 0:
            return True
    return False


def _parse_member_declaration(
    code_part: str,
    comment: str,
    original_line: str,
) -> Optional[_MemberDeclaration]:
    """Parse one 'Type name = init;' / 'Type name{init};' member line.

    Indexes are computed on a length-preserving masked copy of the line
    (literals and inline block comments replaced by spaces) while every
    extracted slice is taken from the original text, so initializers keep
    their exact content. Returns None for anything that is not safely
    recognisable as a single member variable declaration.
    """
    stripped = code_part.strip()
    if not stripped.endswith(";") or "/*" in stripped:
        return None
    masked = _mask_literals(stripped)
    body = masked[:-1].rstrip()
    split = _find_declaration_split(body)
    if split is None:
        return None
    if split >= 0:
        lhs_end = split
        rhs_text = _SPACE_BEFORE_SEMICOLON_REGEX.sub(";", stripped[split:].strip())
        if _has_top_level_comma(_mask_literals(stripped[split:])):
            return None
    else:
        lhs_end = len(body)
        rhs_text = ""
    lhs_masked = body[:lhs_end].rstrip()
    array_start = None
    array_match = _ARRAY_SUFFIX_REGEX.search(lhs_masked)
    if array_match is not None:
        array_start = array_match.start()
        lhs_masked = lhs_masked[:array_start].rstrip()
    name_match = _IDENTIFIER_TAIL_REGEX.search(lhs_masked)
    if name_match is None:
        return None
    name = name_match.group(0)
    if name in _RESERVED_NAMES:
        return None
    type_part = stripped[:name_match.start()].rstrip()
    if not _is_valid_type_part(type_part):
        return None
    array_suffix = stripped[array_start:lhs_end] if array_start is not None else ""
    if rhs_text.startswith("="):
        initializer = rhs_text[1:].lstrip()
        rest = array_suffix + " = " + initializer if initializer else array_suffix + ";"
    elif rhs_text:
        rest = array_suffix + rhs_text
    else:
        rest = array_suffix + ";"
    indent = code_part[:len(code_part) - len(code_part.lstrip())]
    return _MemberDeclaration(
        indent=indent,
        type_part=type_part,
        name=name,
        rest=rest,
        comment=comment.strip(),
        original_line=original_line,
    )


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
        code_part, comment_part, block_comment_state = _split_trailing_comment(
            line, state.in_block_comment
        )
        state.in_block_comment = block_comment_state
        stripped_code = code_part.strip()

        if was_in_block_comment or (not stripped_code and (comment_part or block_comment_state)):
            if stripped_code:
                state.update(_mask_literals(stripped_code))
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
            parsed = _parse_member_declaration(code_part, comment_part, line)

        if parsed is None:
            _flush_block(block, output, max_line_length)
            output.append(line)
        else:
            block.append(_BlockItem(declaration=parsed, raw_line=line))

        state.update(_mask_literals(stripped_code))

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
