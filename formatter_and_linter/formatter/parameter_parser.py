#!/usr/bin/env python3
"""
Parameter Parser

Parsing utilities for C++ function parameters: splits a parameter string into
individual (type, name) tuples.
"""

import re
from typing import List, Optional, Tuple


def parse_parameter(param: str) -> Optional[Tuple[str, str]]:
    """
    Parse a single parameter string into type and name.

    Args:
        param: The parameter string to parse

    Returns:
        A tuple of (type, name) if parsing succeeds, None otherwise
    """
    param = param.strip()

    # Handle [[maybe_unused]] attribute - treat it as part of the type
    if '[[maybe_unused]]' in param:
        # Extract the full type including [[maybe_unused]]
        type_match = re.match(r'^(.*?\[\[maybe_unused\]\].*?)\s+(\w+)$', param)
        if type_match:
            param_type = type_match.group(1).strip()
            param_name = type_match.group(2).strip()
            return (param_type, param_name)

    # Normal parameter parsing
    param_match = re.match(r'^(.+?)\s+(\w+)$', param)

    if param_match:
        param_type = param_match.group(1).strip()
        param_name = param_match.group(2).strip()
        return (param_type, param_name)

    return None


def extract_parameters(params_str: str) -> Optional[List[Tuple[str, str]]]:
    """
    Extract and parse all parameters from a parameter string.

    Args:
        params_str: The parameter string from a function signature

    Returns:
        A list of (type, name) tuples if all parameters parse successfully,
        None if any parameter fails to parse
    """
    params = []
    current_param = ""
    bracket_depth = 0
    square_depth = 0

    for char in params_str:
        if char == '<':
            bracket_depth += 1
            current_param += char
        elif char == '>':
            bracket_depth -= 1
            current_param += char
        elif char == '[':
            square_depth += 1
            current_param += char
        elif char == ']':
            square_depth -= 1
            current_param += char
        elif char == ',' and bracket_depth == 0 and square_depth == 0:
            if current_param.strip():
                params.append(current_param.strip())
            current_param = ""
        else:
            current_param += char

    if current_param.strip():
        params.append(current_param.strip())

    parsed_params = []
    for param in params:
        parsed = parse_parameter(param)
        if parsed:
            parsed_params.append(parsed)
        else:
            return None

    return parsed_params
