#!/usr/bin/env python3
"""
Short If Statement Formatter (With Braces)

Formats if statements that have braces but are still written on a single
line, splitting the condition and each statement onto their own lines.
"""

import re


def find_matching_paren(line: str, opening_pos: int) -> int:
    """
    Find the position of the closing parenthesis that matches the opening one.
    Handles nested parentheses correctly.

    Args:
        line: The code line
        opening_pos: Position of the opening parenthesis

    Returns:
        Position of matching closing parenthesis, or -1 if not found
    """
    if opening_pos < 0 or opening_pos >= len(line) or line[opening_pos] != '(':
        return -1

    paren_depth = 0
    in_string = False
    string_char = None

    for i in range(opening_pos, len(line)):
        char = line[i]

        # Handle string literals
        if char in ('"', "'") and (i == 0 or line[i-1] != '\\'):
            if not in_string:
                in_string = True
                string_char = char
            elif char == string_char:
                in_string = False
                string_char = None

        # Skip parenthesis handling if inside string
        if not in_string:
            if char == '(':
                paren_depth += 1
            elif char == ')':
                paren_depth -= 1
                if paren_depth == 0:
                    return i

    return -1  # No matching closing parenthesis found


def format_short_if_statements_with_braces(code: str) -> str:
    """
    Format if statements that have braces but are still on a single line.

    Args:
        code: The code as a string to format

    Returns:
        The formatted code with properly formatted if statements
    """
    lines = code.splitlines()
    formatted_lines = []
    i = 0

    while i < len(lines):
        line = lines[i]

        # Check for if statements with braces on a single line: if (condition) { statement; }
        if_brace_pattern = r'^\s*if\s*\([^)]+\)\s*\{[^}]+\}\s*$'
        match = re.match(if_brace_pattern, line)

        if match:
            # Extract the condition and the content inside braces
            condition_match = re.search(r'if\s*\(([^)]+)\)', line)
            if condition_match:
                condition = condition_match.group(1)
                # Extract content between braces
                brace_start = line.find('{') + 1
                brace_end = line.rfind('}')
                content = line[brace_start:brace_end].strip()

                # Split content by semicolons and format each statement
                statements = [s.strip() for s in content.split(';') if s.strip()]

                # Preserve original indentation
                indent_match = re.match(r'^(\s*)', line)
                leading_whitespace = indent_match.group(1) if indent_match else ''
                inner_indent = leading_whitespace + '    '

                # Format as multi-line if statement with braces
                formatted_lines.append(f"{leading_whitespace}if ({condition}) {{")
                for statement in statements:
                    if statement:  # Only add non-empty statements
                        formatted_lines.append(f"{inner_indent}{statement};")
                formatted_lines.append(f"{leading_whitespace}}}")
        else:
            formatted_lines.append(line)

        i += 1

    return '\n'.join(formatted_lines)
