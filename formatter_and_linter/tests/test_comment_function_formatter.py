#!/usr/bin/env python3
"""
Comment-Function Spacing Formatter Tests

Unit tests for the comment-function spacing formatter, ensuring no blank line
is inserted between a comment and a function declaration, while blank lines
are preserved (or added) between regular code and functions.

Run with:
   python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.comment_function_formatter import format_comment_function_spacing


def fmt(code: str) -> str:
    return format_comment_function_spacing(code)


class TestCommentFunctionSpacing(unittest.TestCase):

    def test_block_comment_closing_with_no_blank_line_before_function(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), code)

    def test_block_comment_with_extra_blank_line_is_collapsed(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        expected = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), expected)

    def test_block_comment_with_multiple_blank_lines_is_collapsed(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "\n"
            "\n"
            "\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        expected = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), expected)

    def test_single_line_comment_before_function(self):
        code = (
            "// Description...\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), code)

    def test_single_line_block_comment_before_function(self):
        code = (
            "/* Description... */\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), code)

    def test_code_before_function_gets_blank_line(self):
        code = (
            "int x = 5;\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        expected = (
            "int x = 5;\n"
            "\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), expected)

    def test_function_after_function_gets_blank_line(self):
        code = (
            "int setup = 0;\n"
            "static void first()\n"
            "{\n"
            "}\n"
            "static void second()\n"
            "{\n"
            "}"
        )
        expected = (
            "int setup = 0;\n"
            "\n"
            "static void first()\n"
            "{\n"
            "}\n"
            "\n"
            "static void second()\n"
            "{\n"
            "}"
        )
        self.assertEqual(fmt(code), expected)

    def test_existing_blank_line_between_block_comment_and_function_removed(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "\n"
            "static void foo()\n"
            "{\n"
            "}"
        )
        expected = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static void foo()\n"
            "{\n"
            "}"
        )
        self.assertEqual(fmt(code), expected)

    def test_idempotent_when_correct_with_comment(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), code)

    def test_idempotent_when_correct_with_blank_between_code(self):
        code = (
            "int x = 5;\n"
            "\n"
            "static int foo()\n"
            "{\n"
            "    return 0;\n"
            "}"
        )
        self.assertEqual(fmt(code), code)

    def test_block_comment_before_function_at_start_of_file(self):
        code = (
            "/*\n"
            "  Description...\n"
            "*/\n"
            "static void foo()\n"
            "{\n"
            "}"
        )
        self.assertEqual(fmt(code), code)


if __name__ == "__main__":
    unittest.main()
