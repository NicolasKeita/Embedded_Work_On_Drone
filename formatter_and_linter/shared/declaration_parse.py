#!/usr/bin/env python3
"""
Declaration type / name parsing primitives shared by the alignment passes.

Centralises the low-level parsing used to split a single C++ variable
declaration line ('Type name = init;', 'Type name{init};', 'Type name(args);',
'Type name;') into its indentation, the full type part that precedes the
variable name, the variable name and the trailing initializer. Everything is
computed on a length-preserving masked copy of the line so string / char
literals and inline block comments never disturb the brace / separator scans
while the extracted slices stay byte-identical to the source.

Complex types are supported: namespaced names ('a::b::C'), templates
('<...>', nested and containing commas or integer literals), fixed-width types
('std::uint64_t'), qualifiers ('const', 'static', 'constexpr', 'mutable',
'unsigned', ...), pointers and references, array suffixes ('[3]', '[N][M]')
and leading attributes. Declarations that cannot be parsed with certainty
(bit-fields, function pointers, multiple declarators, elaborated type
specifiers, reserved-word names, ...) yield None so callers leave them
untouched.
"""

import re
from typing import NamedTuple, Optional, Tuple


IDENTIFIER_REGEX = re.compile(r"[A-Za-z_]\w*")

IDENTIFIER_TAIL_REGEX = re.compile(r"[A-Za-z_]\w*$")

ARRAY_SUFFIX_REGEX = re.compile(r"(?:\s*\[[^\[\]]*\])+\s*$")

TYPE_TOKEN_REGEX = re.compile(r"[A-Za-z_]\w*|::|[<>*&\[\]]|\S")

INTEGER_LITERAL_REGEX = re.compile(r"-?\d+")

SPACE_BEFORE_SEMICOLON_REGEX = re.compile(r"\s+;$")


NON_TYPE_KEYWORDS = frozenset([
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


RESERVED_NAMES = frozenset([
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


class DeclarationParts(NamedTuple):
    """One parsed variable declaration line ready to be realigned."""

    indent: str
    type_part: str
    name: str
    rest: str
    comment: str
    original_line: str


def mask_literals(line: str) -> str:
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


def split_trailing_comment(line: str, in_block_comment: bool) -> Tuple[str, str, bool]:
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


def find_declaration_split(body: str, allow_paren_init: bool = False) -> Optional[int]:
    """Locate the first top-level '=' or '{' separating 'type name' from its
    initializer.

    Returns the index of the separator, -1 when the declaration carries no
    initializer, or None when the line cannot be a single variable
    declaration (commas / extra semicolons at top level, i.e. methods,
    multiple declarators or function pointers).

    By default a top-level '(' also yields None: inside a class body a '(' is
    ambiguous (it can start a function pointer or a member function). For
    function-local declarations a '(' instead opens a constructor-call
    initializer ('Type name(args);'), which is legal there, so callers can
    pass ``allow_paren_init=True`` to treat it as a split point.
    """
    angle_depth = 0
    for index, char in enumerate(body):
        if char == "<":
            angle_depth += 1
        elif char == ">":
            if angle_depth > 0:
                angle_depth -= 1
        elif angle_depth == 0:
            if char == "(":
                return index if allow_paren_init else None
            if char == ")":
                return None
            if char in (",", ";"):
                return None
            if char in ("=", "{"):
                return index
    return -1


def is_valid_type_part(type_part: str) -> bool:
    """Check that the text before the variable name looks like a C++ type."""
    if not type_part:
        return False
    tokens = TYPE_TOKEN_REGEX.findall(type_part)
    if not tokens:
        return False
    if tokens[0] in NON_TYPE_KEYWORDS:
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
        if IDENTIFIER_REGEX.fullmatch(token):
            continue
        if angle_depth > 0 and INTEGER_LITERAL_REGEX.fullmatch(token):
            continue
        return False
    return angle_depth == 0


def has_top_level_comma(text: str) -> bool:
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


def parse_declaration(
    code_part: str,
    comment: str,
    original_line: str,
    allow_paren_init: bool = False,
) -> Optional[DeclarationParts]:
    """Parse one 'Type name = init;' / 'Type name{init};' / 'Type name(args);'
    declaration line.

    Indexes are computed on a length-preserving masked copy of the line
    (literals and inline block comments replaced by spaces) while every
    extracted slice is taken from the original text, so initializers keep
    their exact content. Returns None for anything that is not safely
    recognisable as a single variable declaration.

    ``allow_paren_init`` enables the constructor-call form 'Type name(args);'
    used by function-local declarations; it is False by default to keep the
    member-alignment pass (class bodies) conservative.
    """
    stripped = code_part.strip()
    if not stripped.endswith(";") or "/*" in stripped:
        return None
    masked = mask_literals(stripped)
    body = masked[:-1].rstrip()
    split = find_declaration_split(body, allow_paren_init)
    if split is None:
        return None
    if split >= 0:
        lhs_end = split
        rhs_text = SPACE_BEFORE_SEMICOLON_REGEX.sub(";", stripped[split:].strip())
        if has_top_level_comma(mask_literals(stripped[split:])):
            return None
    else:
        lhs_end = len(body)
        rhs_text = ""
    lhs_masked = body[:lhs_end].rstrip()
    array_start = None
    array_match = ARRAY_SUFFIX_REGEX.search(lhs_masked)
    if array_match is not None:
        array_start = array_match.start()
        lhs_masked = lhs_masked[:array_start].rstrip()
    name_match = IDENTIFIER_TAIL_REGEX.search(lhs_masked)
    if name_match is None:
        return None
    name = name_match.group(0)
    if name in RESERVED_NAMES:
        return None
    type_part = stripped[:name_match.start()].rstrip()
    if not is_valid_type_part(type_part):
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
    return DeclarationParts(
        indent=indent,
        type_part=type_part,
        name=name,
        rest=rest,
        comment=comment.strip(),
        original_line=original_line,
    )


__all__ = [
    "DeclarationParts",
    "mask_literals",
    "split_trailing_comment",
    "find_declaration_split",
    "is_valid_type_part",
    "has_top_level_comma",
    "parse_declaration",
    "IDENTIFIER_REGEX",
    "IDENTIFIER_TAIL_REGEX",
    "ARRAY_SUFFIX_REGEX",
    "TYPE_TOKEN_REGEX",
    "INTEGER_LITERAL_REGEX",
    "SPACE_BEFORE_SEMICOLON_REGEX",
    "NON_TYPE_KEYWORDS",
    "RESERVED_NAMES",
]
