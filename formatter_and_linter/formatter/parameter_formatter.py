#!/usr/bin/env python3
"""
Parameter Alignment and Formatting

Alignment computation, indentation calculation and parameter list rendering
for C++ function declarations/definitions.
"""

from typing import List, Tuple, Match

from formatter.parameter_parser import extract_parameters

PARAM_NAME_SEPARATOR_WIDTH = 1


def calculate_alignment(parsed_params: List[Tuple[str, str]]) -> Tuple[int, int]:
    """
    Calculate the maximum lengths for type and name alignment.

    Args:
        parsed_params: List of (type, name) tuples

    Returns:
        A tuple of (max_type_length, max_name_length)
    """
    if not parsed_params:
        return 0, 0

    max_type_len = max(len(ptype) for ptype, _ in parsed_params)
    max_name_len = max(len(pname) for _, pname in parsed_params)

    return max_type_len, max_name_len


def calculate_indentation(prefix: str, func_name: str, max_type_len: int, leading_indent: str = "") -> str:
    """
    Calculate the indentation for parameter formatting.

    Args:
        prefix: The function prefix (modifiers, return type)
        func_name: The function name
        max_type_len: Maximum type length for alignment

    Returns:
        The indentation string
    """
    # Calculate the position where the first parameter's type starts
    # This is: prefix + space + function name + opening parenthesis
    first_param_type_start = len(leading_indent) + len(prefix) + 1 + len(func_name) + 1

    # The indentation for subsequent lines should align with the start of the first parameter's type
    return ' ' * first_param_type_start


def format_parameters_list(
    parsed_params: List[Tuple[str, str]],
    indent: str,
    max_type_len: int
) -> str:
    """
    Format a list of parameters with proper alignment.

    Args:
        parsed_params: List of (type, name) tuples
        indent: The indentation string
        max_type_len: Maximum type length for alignment

    Returns:
        Formatted parameter list as a string
    """
    lines = []

    for i, (ptype, pname) in enumerate(parsed_params):
        formatted_type = ptype.ljust(max_type_len)
        separator = ' ' * PARAM_NAME_SEPARATOR_WIDTH

        if i == 0:
            # First parameter stays on the same line as function name
            line = f"({formatted_type}{separator}{pname},"
        elif i < len(parsed_params) - 1:
            # Middle parameters
            line = f"{indent}{formatted_type}{separator}{pname},"
        else:
            # Last parameter
            line = f"{indent}{formatted_type}{separator}{pname})"

        lines.append(line)

    return '\n'.join(lines)


def should_format_function(prefix: str, func_name: str, params_str: str) -> bool:
    """
    Determine if a function should be formatted.

    Args:
        prefix: The function prefix
        func_name: The function name
        params_str: The parameter string

    Returns:
        True if the function should be formatted, False otherwise
    """
    if not params_str.strip():
        return False

    if '\n' not in params_str:
        return False

    return True


def format_single_function(match: Match[str]) -> str:
    """
    Format a single function's parameters.

    Args:
        match: Regex match object containing function components

    Returns:
        Formatted function string
    """
    leading_indent = match.group(1)
    full_prefix = match.group(2)
    func_name = match.group(3)
    params_str = match.group(4)
    signature_suffix = match.group(5).strip()

    if not should_format_function(full_prefix, func_name, params_str):
        return match.group(0)

    parsed_params = extract_parameters(params_str)

    if not parsed_params:
        return match.group(0)

    max_type_len, max_name_len = calculate_alignment(parsed_params)

    indent = calculate_indentation(full_prefix, func_name, max_type_len)

    formatted_params = format_parameters_list(parsed_params, indent, max_type_len)

    const_part = f" {signature_suffix}" if signature_suffix else ""
    result = f"{leading_indent}{full_prefix} {func_name}{formatted_params}{const_part}\n{{"

    return result

