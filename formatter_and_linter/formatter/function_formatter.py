#!/usr/bin/env python3
"""
Function Parameter Formatting

Public entry points for function parameter formatting. The implementation
lives in parameter_parser, parameter_formatter and multiline_function_formatter.

Formats C++ function parameters for readability: handles single-line and
multi-line signatures, parameter alignment and spacing.
"""

import re

from formatter.parameter_parser import parse_parameter, extract_parameters
from formatter.parameter_formatter import (
    PARAM_NAME_SEPARATOR_WIDTH,
    calculate_alignment,
    calculate_indentation,
    format_parameters_list,
    should_format_function,
    format_single_function,
)
from formatter.multiline_function_formatter import format_multiline_function_params


def format_function_params(code: str) -> str:
    """
    Format function parameters in C++ code for better readability.

    Args:
        code: The C++ code as a string

    Returns:
        The formatted code
    """
    # First handle single-line function definitions.
    # The match is anchored to the start of a line and parameters cannot
    # cross ';' or braces, so preprocessor directives (e.g. '#if defined(_WIN32)')
    # are never consumed as part of a signature.
    pattern = (
        r'^([ \t]*)'
        r'((?:(?:static|inline|virtual|explicit|constexpr|const)\s+)*[\w:]+(?:\s*[*&])*)'
        r'\s+(\w+)\s*\(([^;{}]*?)\)\s*'
        r'((?:const\b\s*|noexcept\b(?:\s*\([^()]*\))?\s*|override\b\s*|final\b\s*)*)'
        r'\{'
    )

    formatted = re.sub(pattern, format_single_function, code, flags=re.MULTILINE)

    # Then handle multi-line function definitions
    formatted = format_multiline_function_params(formatted)

    return formatted


__all__ = [
    "parse_parameter",
    "extract_parameters",
    "PARAM_NAME_SEPARATOR_WIDTH",
    "calculate_alignment",
    "calculate_indentation",
    "format_parameters_list",
    "should_format_function",
    "format_single_function",
    "format_multiline_function_params",
    "format_function_params",
]
