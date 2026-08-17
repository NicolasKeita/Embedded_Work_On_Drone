#!/usr/bin/env python3
"""
Import classification and sorting utilities for C++20 modules.

Provides helpers to determine if an import is system or local,
extract module names, sort hierarchically, and deduplicate imports.
"""

import re
from typing import List, Tuple

from shared.regex_patterns import IMPORT_MODULE_EXTRACT_REGEX


def is_system_import(import_line: str) -> bool:
    match = re.match(IMPORT_MODULE_EXTRACT_REGEX, import_line)
    if not match:
        return False
    module_name = match.group(1).strip()
    if module_name.startswith('<') and module_name.endswith('>'):
        return True
    first_identifier = module_name.split('.')[0].strip()
    if not first_identifier:
        return True
    if first_identifier[0].islower():
        return True
    return False


def extract_import_module(import_line: str) -> str:
    match = re.match(IMPORT_MODULE_EXTRACT_REGEX, import_line)
    if match:
        return match.group(1).strip()
    return ""


def _hierarchical_sort_key(module_name: str) -> List[str]:
    if module_name.startswith('<') and module_name.endswith('>'):
        return [module_name.casefold()]
    return [segment.casefold() for segment in module_name.split('.')]


def sort_imports(imports: List[str]) -> Tuple[List[str], List[str]]:
    seen_modules = set()
    system_imports = []
    local_imports = []
    for imp in imports:
        module_name = extract_import_module(imp)
        if not module_name:
            continue
        if module_name in seen_modules:
            continue
        seen_modules.add(module_name)
        if is_system_import(imp):
            system_imports.append(imp)
        else:
            local_imports.append(imp)
    system_imports.sort(key=lambda x: _hierarchical_sort_key(extract_import_module(x)))
    local_imports.sort(key=lambda x: _hierarchical_sort_key(extract_import_module(x)))
    return system_imports, local_imports