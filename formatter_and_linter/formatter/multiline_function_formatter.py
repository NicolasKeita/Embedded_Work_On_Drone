#!/usr/bin/env python3
"""
Multi-line Function Parameter Formatter

Re-formats C++ function definitions whose parameters are already spread over
several lines.
"""

import re

from formatter.parameter_parser import extract_parameters
from formatter.parameter_formatter import (
    calculate_alignment,
    calculate_indentation,
    format_parameters_list,
)


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

                    # Remove trailing ) but keep the signature suffix that
                    # follows it (noexcept, const, override, trailing ; or {)
                    signature_suffix = ''
                    if ')' in all_params:
                        closing_index = all_params.rfind(')')
                        signature_suffix = all_params[closing_index + 1:]
                        all_params = all_params[:closing_index]

                    inline_body = ''
                    if '{' in signature_suffix:
                        suffix_part, inline_body = signature_suffix.split('{', 1)
                    else:
                        suffix_part = signature_suffix

                    signature_suffix = suffix_part.strip().rstrip(';').strip()

                    # Parse and format parameters
                    parsed_params = extract_parameters(all_params)

                    if parsed_params and len(parsed_params) > 1:
                        max_type_len, _ = calculate_alignment(parsed_params)
                        indent = calculate_indentation(full_prefix, full_func_name, max_type_len, leading_indent)
                        formatted_params = format_parameters_list(parsed_params, indent, max_type_len)

                        const_part = f" {signature_suffix}" if signature_suffix else ""

                            # Add formatted function declaration
                        if has_brace:
                            result_lines.append(f"{leading_indent}{full_prefix} {full_func_name}"
                                                f"{formatted_params}{const_part}")
                            result_lines.append(f"{leading_indent}{{")

                            # Handle content after { on last param line (only if { is in that line)
                            if inline_body:
                                result_lines.append(inline_body.strip())
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
