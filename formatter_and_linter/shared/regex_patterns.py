#!/usr/bin/env python3
"""
Shared regex patterns for the C++ code formatter.

Centralizes commonly used regex patterns to avoid duplication across modules.
"""

FUNC_REGEX = r'^\s*static\s+\w[\w:<>\s*&]*\s+\w+\s*\([^)]*\)\s*'

INCLUDE_REGEX = r'^\s*#\s*include\s+(<[^>]+>|"[^"]+")\s*$'

MODULE_DECL_REGEX = r'^\s*module\s+[\w:.]+\s*;\s*$'

MODULE_PARTITION_REGEX = r'^\s*module\s+[\w:.]+:\w+\s*;\s*$'

IMPORT_REGEX = r'^\s*import\s+[\w:.<>]+\s*;\s*$'

USING_REGEX = r'^\s*using\s+'

IMPORT_MODULE_EXTRACT_REGEX = r'^\s*import\s+([^;]+)\s*;\s*$'