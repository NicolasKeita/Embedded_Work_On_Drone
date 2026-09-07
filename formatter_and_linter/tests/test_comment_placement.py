#!/usr/bin/env python3
"""
Comment Placement Check Tests

Unit tests for the comment placement rule: a comment block (single-line,
contiguous multi-line `//`, or `/* */`) is valid only when it sits directly
above a function definition or a top-level declaration.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from shared.comment_utils import check_comment_placement, detect_comments_and_functions


def placement_violations(code: str):
    comments, function_lines, declaration_lines = detect_comments_and_functions(code)
    return check_comment_placement(comments, function_lines, declaration_lines)


class TestCommentPlacement(unittest.TestCase):

    def test_single_comment_above_function_is_valid(self):
        code = (
            "// doc\n"
            "int get_bar()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_multiline_slash_slash_comment_above_function_is_valid(self):
        code = (
            "// doc line 1\n"
            "// doc line 2\n"
            "int get_bar()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_noexcept_function_is_valid(self):
        code = (
            "// doc\n"
            "int get_bar() noexcept\n"
            "{\n"
            "    return 1;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_multiline_comment_above_constexpr_variable_is_valid(self):
        code = (
            "// constexpr so the registry is constant-initialized: it can be\n"
            "// safely read during dynamic initialization of other catalogs.\n"
            "constexpr std::array<int, 2> kFoo{{1, 2}};\n"
            "\n"
            "int get_foo()\n"
            "{\n"
            "    return kFoo[0];\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_block_comment_above_function_is_valid(self):
        code = (
            "/*\n"
            "doc\n"
            "*/\n"
            "int get_bar()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_inside_function_body_above_local_is_invalid(self):
        code = (
            "int get_bar()\n"
            "{\n"
            "    // note\n"
            "    int local = 1;\n"
            "    return local;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [(3, "singleline")])

    def test_comment_inside_function_body_above_statement_is_invalid(self):
        code = (
            "int get_bar()\n"
            "{\n"
            "    int local = 1;\n"
            "    // note\n"
            "    return local;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [(4, "singleline")])

    def test_floating_comment_not_above_declaration_is_invalid(self):
        code = (
            "int get_bar()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "// floating note\n"
            "\n"
            "constexpr int kFoo{1};\n"
        )
        self.assertEqual(placement_violations(code), [(6, "singleline")])

    def test_header_comment_in_first_lines_is_valid(self):
        code = (
            "// file header note\n"
            "\n"
            "constexpr int kFoo{1};\n"
            "\n"
            "int get_foo()\n"
            "{\n"
            "    return kFoo;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_type_declaration_is_valid(self):
        code = (
            "// scenario descriptor\n"
            "struct Scenario\n"
            "{\n"
            "    int id;\n"
            "};\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_nodiscard_function_is_valid(self):
        code = (
            "/*\n"
            "doc\n"
            "*/\n"
            "[[nodiscard]] int get_bar()\n"
            "{\n"
            "    return 1;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_template_nodiscard_function_is_valid(self):
        code = (
            "/*\n"
            "doc\n"
            "*/\n"
            "template<typename Number>\n"
            "[[nodiscard]] Number get_bar(Number n)\n"
            "{\n"
            "    return n;\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_nodiscard_declaration_is_valid(self):
        code = (
            "/*\n"
            "doc\n"
            "*/\n"
            "[[nodiscard]] int get_bar();\n"
        )
        self.assertEqual(placement_violations(code), [])

    def test_comment_above_nodiscard_namespace_function_is_valid(self):
        code = (
            "namespace sim::hil {\n"
            "/*\n"
            "doc\n"
            "*/\n"
            "    [[nodiscard]] int get_bar()\n"
            "    {\n"
            "        return 1;\n"
            "    }\n"
            "}\n"
        )
        self.assertEqual(placement_violations(code), [])


if __name__ == "__main__":
    unittest.main()
