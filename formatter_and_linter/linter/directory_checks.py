#!/usr/bin/env python3
"""
Directory Checks

Checks the number of source files (.cpp/.cppm) per directory and reports
directories that hold too many files and should be split.
"""

import os
from typing import Dict, List, Tuple

MAX_FILES_PER_DIRECTORY = 8
SOURCE_EXTENSIONS = ('.cpp', '.cppm')


def count_source_files_per_directory(directories: List[str]) -> Dict[str, int]:
    counts = {}
    for directory in directories:
        for root, _, files in os.walk(directory):
            source_count = sum(1 for file in files if file.endswith(SOURCE_EXTENSIONS))
            if source_count > 0:
                counts[root] = counts.get(root, 0) + source_count
    return counts


def check_directory_file_counts(directories: List[str], max_files: int = MAX_FILES_PER_DIRECTORY) -> List[Tuple[str, int]]:
    counts = count_source_files_per_directory(directories)
    return sorted(
        (directory, count) for directory, count in counts.items() if count > max_files
    )
