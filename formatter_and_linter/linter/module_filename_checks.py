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
"""

import os
from typing import List, Tuple

INTERFACE_EXTENSION = ".cppm"
IMPLEMENTATION_EXTENSION = ".cpp"


def _find_implementation_files(files: List[str], module_name: str) -> List[str]:
    return sorted(
        file for file in files
        if file.endswith(IMPLEMENTATION_EXTENSION) and file.startswith(module_name)
    )


def _suggest_hyphenated_name(module_name: str, file_name: str) -> str:
    suffix = file_name[len(module_name):]
    if suffix == IMPLEMENTATION_EXTENSION:
        return module_name + "-Core" + IMPLEMENTATION_EXTENSION
    return module_name + "-" + suffix[:-len(IMPLEMENTATION_EXTENSION)] + IMPLEMENTATION_EXTENSION


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
            for interface_file in interface_files:
                module_name = interface_file[:-len(INTERFACE_EXTENSION)]
                interface_path = os.path.join(root, interface_file)
                implementations = _find_implementation_files(files, module_name)

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
                    suffix = file_name[len(module_name):]
                    if not suffix.startswith("-"):
                        suggestion = _suggest_hyphenated_name(module_name, file_name)
                        violations.append((
                            os.path.join(root, file_name),
                            "multiple implementations of '" + module_name + "' must follow '"
                            + module_name + "-<Suffix>.cpp' (e.g. '" + suggestion + "')",
                        ))

    return sorted(violations)