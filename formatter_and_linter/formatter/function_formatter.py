#!/usr/bin/env python3
"""
Function parameter formatting utilities for the C++ code formatter.

This module provides functions for formatting function parameters in C++ code
to improve readability and consistency. Handles parameter alignment, spacing,
and formatting for both function declarations and definitions with proper
regex-based pattern matching and replacement.
"""

import re
from typing import List, Tuple, Optional, Match

PARAM_NAME_SEPARATOR_WIDTH = 1


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
    full_prefix = match.group(1)
    func_name = match.group(2)
    params_str = match.group(3)
    const_qualifier = match.group(4) or ""
    
    if not should_format_function(full_prefix, func_name, params_str):
        return match.group(0)
    
    parsed_params = extract_parameters(params_str)
    
    if not parsed_params:
        return match.group(0)
    
    max_type_len, max_name_len = calculate_alignment(parsed_params)
    
    indent = calculate_indentation(full_prefix, func_name, max_type_len)
    
    formatted_params = format_parameters_list(parsed_params, indent, max_type_len)
    
    const_part = f" {const_qualifier}" if const_qualifier.strip() else ""
    result = f"{full_prefix} {func_name}{formatted_params}{const_part}\n{{"
    
    return result


def format_multiline_function_params(code: str) -> str:
    """
    Format multi-line function parameters in C++ code for better readability.
    This handles cases where function parameters are already on multiple lines.
    
    Args:
        code: The C++ code as a string
        
    Returns:
        The formatted code
    """
    lines = code.splitlines()
    result_lines = []
    i = 0
    
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        # Look for function signature start: return_type ClassName::function_name(
        # This regex handles:
        # - Simple: return_type function_name(
        # - With namespace: return_type Namespace::function_name(
        # - With class: return_type Class::function_name(
        # - With qualifiers: static inline return_type Class::function_name(
        match = re.match(
            r'^(\s*)((?:(?:static|inline|virtual|explicit|constexpr|const)\s+)*[\w:]+(?:\s*[*&])*)\s+([\w:]+)\s*\(',
            line
        )
        
        if match:
            leading_indent = match.group(1)
            full_prefix = match.group(2)
            # Get only the function name (last part after ::)
            full_func_name = match.group(3)
            func_name = full_func_name.split('::')[-1]
            
            # Count parentheses to find the end of the parameter list
            paren_depth = stripped.count('(') - stripped.count(')')
            
            if paren_depth > 0:
                # Multi-line parameters - collect all lines until parentheses are balanced
                param_lines = [line]
                j = i + 1
                
                while j < len(lines) and paren_depth > 0:
                    next_line = lines[j].strip()
                    paren_depth += next_line.count('(') - next_line.count(')')
                    param_lines.append(lines[j])
                    j += 1
                
                # Check if the last param line has ) and possibly {
                last_param_line = param_lines[-1].strip()
                
                # Check if this is a function prototype (ends with ;)
                # But also check if next line is just a brace (definition, not prototype)
                is_prototype = last_param_line.endswith(';')
                next_line_is_brace = j < len(lines) and lines[j].strip() == '{'
                if is_prototype and next_line_is_brace:
                    is_prototype = False

                # Check if this line has the closing ) and possibly {
                if paren_depth == 0:
                    # Extract the full parameter string from collected lines
                    # Remove the function prefix from first line
                    first_line_params = line[match.end():]
                    
                    # Collect all parameter content
                    all_params = first_line_params
                    for k in range(1, len(param_lines)):
                        all_params += ' ' + param_lines[k].strip()
                    
                    # Check if the last line has ) and {
                    has_brace = '{' in last_param_line

                    # If this was a prototype but next line is brace, it's a definition
                    # with semicolon that needs to be removed
                    if not has_brace and next_line_is_brace:
                        has_brace = True

                    brace_on_next_line = has_brace and '{' not in last_param_line and next_line_is_brace
                    
                    # Remove trailing ) and anything after (like { or ;)
                    if ')' in all_params:
                        all_params = all_params[:all_params.rfind(')')]
                    
                    # Parse and format parameters
                    parsed_params = extract_parameters(all_params)
                    
                    if parsed_params and len(parsed_params) > 1:
                        max_type_len, _ = calculate_alignment(parsed_params)
                        indent = calculate_indentation(full_prefix, full_func_name, max_type_len, leading_indent)
                        formatted_params = format_parameters_list(parsed_params, indent, max_type_len)

                        const_qualifier = ""
                        if has_brace:
                            # Check for const qualifier
                            if '{' in last_param_line:
                                const_match = re.search(r'\)\s*(const)?\s*\{', last_param_line)
                                if const_match and const_match.group(1):
                                    const_qualifier = const_match.group(1)
                            else:
                                const_match = re.search(r'\)\s*(const)?\s*;?$', last_param_line)
                                if const_match and const_match.group(1):
                                    const_qualifier = const_match.group(1)
                        
                        const_part = f" {const_qualifier}" if const_qualifier else ""

                        # Add formatted function declaration
                        if has_brace:
                            result_lines.append(f"{leading_indent}{full_prefix} {full_func_name}"
                                                f"{formatted_params}{const_part}")
                            result_lines.append(f"{leading_indent}{{")
                            
                            # Handle content after { on last param line (only if { is in that line)
                            if '{' in last_param_line:
                                after_brace = last_param_line.split('{', 1)[1].strip()
                                if after_brace:
                                    result_lines.append(after_brace)
                        else:
                            # Function prototype - just add semicolon
                            result_lines.append(f"{leading_indent}{full_prefix} {full_func_name}{formatted_params};")

                        if has_brace:
                            if brace_on_next_line:
                                i = j + 1
                            else:
                                i = j
                        else:
                            i = j
                        continue
            
            # If we didn't handle it as multi-line, add the line as-is
            result_lines.append(line)
        else:
            result_lines.append(line)
        
        i += 1
    
    return '\n'.join(result_lines)


def format_function_params(code: str) -> str:
    """
    Format function parameters in C++ code for better readability.
    
    Args:
        code: The C++ code as a string
        
    Returns:
        The formatted code
    """
    # First handle single-line function definitions
    pattern = r'((?:(?:static|inline|virtual|explicit|constexpr|const)\s+)*[\w:]+(?:\s*[*&])*)\s+(\w+)\s*\((.*?)\)\s*(const)?\s*\{'
    
    formatted = re.sub(pattern, format_single_function, code, flags=re.MULTILINE | re.DOTALL)
    
    # Then handle multi-line function definitions
    formatted = format_multiline_function_params(formatted)
    
    return formatted
