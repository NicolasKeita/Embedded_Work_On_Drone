#!/usr/bin/env python3
"""
Short If Statement Formatter

A formatter that ensures if statements are not placed on a single line,
following the coding style rule: AllowShortIfStatementsOnASingleLine: false.
Handles both simple if statements and if statements with braces on single lines.

The with-braces handling lives in if_brace_formatter.
"""

import re

from formatter.if_brace_formatter import (
    find_matching_paren,
    format_short_if_statements_with_braces,
)


def format_short_if_statements(code: str) -> str:
    """
    Format if statements to ensure they are not on a single line.
    Only format if statements that have code ON THE SAME LINE as the condition.
    Allow instructions after if without brackets if they are on the next line (no changes needed).

    Args:
        code: The code as a string to format

    Returns:
        The formatted code with if statements properly split across lines
    """
    lines = code.splitlines()
    formatted_lines = []

    for line in lines:
        # Check if line contains an if statement
        if_match = re.search(r'^\s*if\s*\(', line)

        if if_match:
            # Find the matching closing parenthesis for the condition
            opening_paren_pos = if_match.end() - 1  # Position of opening (
            closing_paren_pos = find_matching_paren(line, opening_paren_pos)

            if closing_paren_pos != -1:
                # Get the content after the condition's closing parenthesis
                after_condition = line[closing_paren_pos + 1:].strip()

                # Only format if there's code after the condition AND no opening brace
                # (condition followed by statement on same line)
                if after_condition and not after_condition.startswith('{'):
                    # This is a single-line if with statement - needs formatting
                    if_part = line[:closing_paren_pos + 1]
                    statement = after_condition.rstrip(';').strip()

                    # Preserve original indentation (extract leading whitespace from the line)
                    indent_match = re.match(r'^(\s*)', line)
                    leading_whitespace = indent_match.group(1) if indent_match else ''
                    inner_indent = leading_whitespace + '    '

                    # Format as multi-line if statement with proper indentation
                    formatted_lines.append(f"{if_part} {{")
                    formatted_lines.append(f"{inner_indent}{statement};")
                    formatted_lines.append(f"{leading_whitespace}}}")
                else:
                    # No statement on same line, or already has braces - keep as is
                    formatted_lines.append(line)
            else:
                formatted_lines.append(line)
        else:
            formatted_lines.append(line)

    return '\n'.join(formatted_lines)


def format_if_statements(code: str) -> str:
    """
    Main function to format all types of short if statements.

    Args:
        code: The code as a string to format

    Returns:
        The formatted code with all if statements properly formatted
    """
    # First pass: handle if statements without braces
    formatted_code = format_short_if_statements(code)

    # Second pass: handle if statements with braces on single line
    formatted_code = format_short_if_statements_with_braces(formatted_code)

    return formatted_code


__all__ = [
    "find_matching_paren",
    "format_short_if_statements",
    "format_short_if_statements_with_braces",
    "format_if_statements",
]


if __name__ == "__main__":
    # Test the formatter with sample code
    test_code = """#include <iostream>

int main() {
    int x = 5;
    if (x > 0) return x;
    if (x < 0) { std::cout << "negative"; }
    if (x == 5) std::cout << "five";
    return 0;
}"""

    print("Original code:")
    print(test_code)
    print("\nFormatted code:")
    print(format_if_statements(test_code))
