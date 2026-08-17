#!/usr/bin/env python3
"""
File handling utilities for the C++ code formatter.

This module provides functions for reading input files, writing output files,
and parsing command-line arguments. Handles file I/O operations and argument
parsing for the formatter application.
"""

import glob
import os
import sys
from typing import Tuple, List


def find_source_files(directory: str, include_hpp: bool = True, include_cppm: bool = False) -> List[str]:
    """
    Find source files in a directory recursively.

    Args:
        directory: Root directory to search
        include_hpp: Whether to include .hpp files (default: True)
        include_cppm: Whether to include .cppm files (default: False)

    Returns:
        List of paths to source files
    """
    cpp_files = glob.glob(os.path.join(directory, '**', '*.cpp'), recursive=True)
    cppm_files = glob.glob(os.path.join(directory, '**', '*.cppm'), recursive=True) if include_cppm else []
    if include_hpp:
        hpp_files = glob.glob(os.path.join(directory, '**', '*.hpp'), recursive=True)
        return sorted(cpp_files + hpp_files + cppm_files)
    else:
        return sorted(cpp_files + cppm_files)


def read_input_file(filepath: str) -> str:
    """
    Read the content of an input file.
    
    Args:
        filepath: Path to the file to read
        
    Returns:
        The content of the file as a string
        
    Raises:
        SystemExit: If the file cannot be read
    """
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except FileNotFoundError:
        print(f"Error: File '{filepath}' does not exist.", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error reading file: {e}", file=sys.stderr)
        sys.exit(1)


def write_output_file(filepath: str, content: str) -> None:
    """
    Write content to an output file.
    
    Args:
        filepath: Path to the file to write
        content: Content to write to the file
        
    Raises:
        SystemExit: If the file cannot be written
    """
    try:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.write(content)
            if not content.endswith('\n'):
                f.write('\n')
        #print(f"Formatted file written to: {filepath}")
    except Exception as e:
        print(f"Error writing file: {e}", file=sys.stderr)
        sys.exit(1)


def parse_arguments() -> Tuple[bool, bool, bool, List[str]]:
    """
    Parse command-line arguments.

    Returns:
        A tuple containing:
        - in_place: Whether to modify the file in place (bool)
        - check_only: Whether to only check without modifying (bool)
        - recursive: Whether to search recursively in Src/ for .cpp files (bool)
        - input_files: List of paths to input files (List[str])
    """
    in_place = False
    check_only = False
    recursive = False
    input_files = []

    args = sys.argv[1:]

    i = 0
    while i < len(args):
        if args[i] == '-i':
            in_place = True
            i += 1
        elif args[i] == '--check':
            check_only = True
            i += 1
        elif args[i] == '--recursive' or args[i] == '-r':
            recursive = True
            i += 1
        else:
            input_files.append(args[i])
            i += 1

    return in_place, check_only, recursive, input_files
