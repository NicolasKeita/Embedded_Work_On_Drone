#!/usr/bin/env python3
"""
Module Filename Checks

Checks the naming convention between module interfaces (.cppm) and their
implementation files (.cpp):

- A single implementation file must be exactly '<ModuleName>.cpp'.
- A missing implementation file is not reported here: the inline function
  body check ([WARN_CPPM_INLINE_FUNCTION]) already indicates when an
  implementation .cpp file is needed.
- Multiple implementation files must all follow '<ModuleName>-<Suffix>.cpp'
  and none may be the plain '<ModuleName>.cpp'.
- A file is an implementation of '<ModuleName>' only when it actually
  declares 'module <ModuleName>;' inside its content. Files that merely
  import the module or share a filename prefix (e.g. 'HilRunnerMain.cpp'
  importing 'HilRunner') are consumers, not implementations.
- Files without any module declaration (e.g. empty files) fall back to
  filename matching: exactly '<ModuleName>.cpp' or '<ModuleName>-*.cpp'.
"""

import os
import re
from typing import Dict, List, Optional, Tuple

INTERFACE_EXTENSION = ".cppm"
IMPLEMENTATION_EXTENSION = ".cpp"

_MODULE_DECLARATION_PATTERN = re.compile(r"^\s*module\s+([A-Za-z_]\w*)\s*;")


def _declared_module(file_path: str) -> Optional[str]:
    """
    Return the name declared by a 'module <Name>;' unit in the file, or None.

    Comment blocks are skipped so that a 'module' word appearing inside the
    header description is not mistaken for a module declaration.
    """
    try:
        with open(file_path, "r", encoding="utf-8", errors="replace") as handle:
            in_block_comment = False
            for line in handle:
                stripped = line
                if in_block_comment:
                    end_index = stripped.find("*/")
                    if end_index == -1:
                        continue
                    stripped = stripped[end_index + len("*/"):]
                    in_block_comment = False
                start_index = stripped.find("/*")
                if start_index != -1:
                    head = stripped[:start_index]
                    match = _MODULE_DECLARATION_PATTERN.match(head)
                    if match:
                        return match.group(1)
                    end_index = stripped.find("*/", start_index + len("/*"))
                    if end_index == -1:
                        in_block_comment = True
                        continue
                    stripped = stripped[end_index + len("*/"):]
                match = _MODULE_DECLARATION_PATTERN.match(stripped)
                if match:
                    return match.group(1)
    except (OSError, ValueError):
        return None
    return None


def _declared_modules(directory: str, files: List[str]) -> Dict[str, Optional[str]]:
    return {
        file: _declared_module(os.path.join(directory, file))
        for file in files
        if file.endswith(IMPLEMENTATION_EXTENSION)
    }


def _find_implementation_files(
    files: List[str],
    module_name: str,
    declared: Dict[str, Optional[str]],
) -> List[str]:
    candidates = []
    base_name = module_name + IMPLEMENTATION_EXTENSION
    for file in files:
        if not file.endswith(IMPLEMENTATION_EXTENSION):
            continue
        if declared.get(file) == module_name:
            candidates.append(file)
        elif declared.get(file, "missing") is None and (
            file == base_name or file.startswith(module_name + "-")
        ):
            candidates.append(file)
    return sorted(candidates)


def _suggest_hyphenated_name(module_name: str, file_name: str) -> str:
    if file_name == module_name + IMPLEMENTATION_EXTENSION:
        return module_name + "-Core" + IMPLEMENTATION_EXTENSION
    return module_name + "-" + file_name[:-len(IMPLEMENTATION_EXTENSION)] + IMPLEMENTATION_EXTENSION


def check_module_filename_convention(directories: List[str]) -> List[Tuple[str, str]]:
    """
    Validate module interface / implementation filename consistency.

    Returns violations as (file_path, message) tuples sorted by path.
    """
    violations = []
    for directory in directories:
        for root, _, files in os.walk(directory):
            interface_files = [
                file for file in files if file.endswith(INTERFACE_EXTENSION)
            ]
            declared = _declared_modules(root, files)
            for interface_file in interface_files:
                module_name = interface_file[:-len(INTERFACE_EXTENSION)]
                implementations = _find_implementation_files(files, module_name, declared)

                if len(implementations) == 0:
                    continue

                if len(implementations) == 1:
                    expected_name = module_name + IMPLEMENTATION_EXTENSION
                    if implementations[0] != expected_name:
                        violations.append((
                            os.path.join(root, implementations[0]),
                            "single implementation of '" + module_name + "' must be named '"
                            + expected_name + "'",
                        ))
                    continue

                base_name = module_name + IMPLEMENTATION_EXTENSION
                for file_name in implementations:
                    if file_name == base_name or not file_name.startswith(module_name + "-"):
                        suggestion = _suggest_hyphenated_name(module_name, file_name)
                        violations.append((
                            os.path.join(root, file_name),
                            "multiple implementations of '" + module_name + "' must follow '"
                            + module_name + "-<Suffix>.cpp' (e.g. '" + suggestion + "')",
                        ))

    return sorted(violations)