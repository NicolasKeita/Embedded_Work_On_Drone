#!/usr/bin/env python3
"""
Prototype and Import Detection

Detection helpers used by the module formatter to classify top-level lines
as includes/modules/imports or function prototypes.
"""

import re

from shared.regex_patterns import (
    MODULE_DECL_REGEX,
    MODULE_PARTITION_REGEX,
    IMPORT_REGEX,
)


def is_import_or_include_or_module(line: str) -> bool:
    stripped = line.strip()
    if re.match(r'^\s*#\s*include\s+', line):
        return True
    if re.match(MODULE_DECL_REGEX, line) or re.match(MODULE_PARTITION_REGEX, line):
        return True
    if re.match(IMPORT_REGEX, line):
        return True
    return False


def is_function_prototype(line: str) -> bool:
    stripped = line.strip()
    if not stripped.endswith(';'):
        return False
    if '(' not in stripped or ')' not in stripped:
        return False
    if '{' in stripped:
        return False
    if stripped.startswith('#'):
        return False
    if stripped.startswith('//') or stripped.startswith('/*'):
        return False
    if re.match(r'^\s*typedef\s+', stripped):
        return False
    if re.match(r'^\s*using\s+', stripped):
        return False
    if re.match(r'^\s*(static_assert|noexcept|throw)\s*\(', stripped):
        return False
    if re.search(r'\)\s*=\s*(delete|default)\s*;', stripped):
        return False
    if not re.search(r'\w\s*\(', stripped):
        return False
    return True


def is_function_prototype_start(line: str) -> bool:
    stripped = line.strip()
    if '(' not in stripped:
        return False
    if '{' in stripped:
        return False
    if stripped.startswith('#'):
        return False
    if stripped.startswith('//') or stripped.startswith('/*'):
        return False
    if re.match(r'^\s*typedef\s+', stripped):
        return False
    if re.match(r'^\s*using\s+', stripped):
        return False
    if re.match(r'^\s*(static_assert|noexcept|throw)\s*\(', stripped):
        return False
    if not re.search(r'\w\s*\(', stripped):
        return False
    if stripped.endswith(';'):
        return False
    return True
