#!/usr/bin/env python3
"""
Unit tests for the .cppm inline function body check
(WARN_CPPM_INLINE_FUNCTION).

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.cppm_inline_function_checks import (
    MAX_CPPM_INLINE_FUNCTION_BODY_LINES,
    WARN_CPPM_INLINE_FUNCTION_TAG,
    check_cppm_inline_function_bodies,
    check_cppm_inline_functions,
    format_cppm_inline_function_message,
)

FILE_HEADER = """/*
Filename: Src/Dummy/Sample.cppm
Description: Sample module interface used by the unit tests.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module Sample;

import std;

"""


class TestCheckCppmInlineFunctions(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)

    def tearDown(self):
        self.temp_dir.cleanup()

    def write_file(self, file_name: str, content: str) -> Path:
        file_path = self.root / file_name
        file_path.write_text(content, encoding="utf-8")
        return file_path

    def write_cppm(self, body: str) -> Path:
        return self.write_file("Sample.cppm", FILE_HEADER + body)

    def test_max_body_lines_constant(self):
        self.assertEqual(MAX_CPPM_INLINE_FUNCTION_BODY_LINES, 1)

    def test_non_cppm_files_are_ignored(self):
        cpp_file = self.write_file("Sample.cpp", "int f()\n{\n    int a = 0;\n    return a;\n}\n")
        self.assertEqual(check_cppm_inline_functions(cpp_file), [])

    def test_single_line_getter_is_allowed(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::int32_t value() const { return value_; }\n"
            "\n"
            "private:\n"
            "    std::int32_t value_ = 0;\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_single_line_setter_is_allowed(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    void setValue(std::int32_t value) { value_ = value; }\n"
            "\n"
            "private:\n"
            "    std::int32_t value_ = 0;\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_empty_body_is_allowed(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    void noop() {}\n"
            "    void alsoNoop() {\n"
            "    }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_multi_line_free_function_is_reported(self):
        file_path = self.write_cppm(
            "export std::int32_t compute(std::int32_t a, std::int32_t b)\n"
            "{\n"
            "    std::int32_t sum = a + b;\n"
            "    return sum * 2;\n"
            "}\n"
        )
        warnings = check_cppm_inline_functions(file_path)
        self.assertEqual(len(warnings), 1)
        message = warnings[0]
        self.assertIn(WARN_CPPM_INLINE_FUNCTION_TAG, message)
        self.assertIn(str(file_path), message)
        self.assertIn(":13 :", message)
        self.assertIn("'compute'", message)
        self.assertIn("2 lignes", message)

    def test_multi_line_method_inside_class_is_reported(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    void reset()\n"
            "    {\n"
            "        stop();\n"
            "        clear();\n"
            "    }\n"
            "};\n"
        )
        warnings = check_cppm_inline_functions(file_path)
        self.assertEqual(len(warnings), 1)
        self.assertIn(":16 :", warnings[0])
        self.assertIn("'reset'", warnings[0])

    def test_multi_line_struct_without_functions_is_not_reported(self):
        file_path = self.write_cppm(
            "export struct SimulationState\n"
            "{\n"
            "    bool fc1_alive = true;\n"
            "    bool comms_link_up = true;\n"
            "    std::float64_t comms_loss_probability = 0.0;\n"
            "    std::float64_t actuator_efficiency = 1.0;\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_multi_line_namespace_without_functions_is_not_reported(self):
        file_path = self.write_cppm(
            "export namespace sim::sample {\n"
            "\n"
            "struct First {\n"
            "    bool alive = true;\n"
            "};\n"
            "\n"
            "struct Second {\n"
            "    std::int32_t count = 0;\n"
            "};\n"
            "\n"
            "}\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_multi_line_enum_class_is_not_reported(self):
        file_path = self.write_cppm(
            "export enum class FaultDomain\n"
            "{\n"
            "    None,\n"
            "    FC1Heartbeat,\n"
            "    CommsLink,\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_multi_line_aggregate_initialization_is_not_reported(self):
        file_path = self.write_cppm(
            "export constexpr std::int32_t kTable[4] = {\n"
            "    1,\n"
            "    2,\n"
            "    3,\n"
            "    4,\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_braces_inside_line_comment_are_ignored(self):
        file_path = self.write_cppm(
            "// void fake() {\n"
            "//     int a = 0;\n"
            "// }\n"
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::int32_t value() const { return 0; } // trailing brace: }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_braces_inside_block_comment_are_ignored(self):
        file_path = self.write_cppm(
            "/*\n"
            "void alsoFake() {\n"
            "    int b = 1;\n"
            "    int c = 2;\n"
            "}\n"
            "*/\n"
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::int32_t value() const { return 0; }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_braces_inside_string_literal_are_ignored(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::string_view text() const { return \"{ not a block }\"; }\n"
            "    std::string_view escaped() const { return \"quote \\\" } here\"; }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_braces_inside_char_literal_are_ignored(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    char openBrace() const { return '{'; }\n"
            "    char closeBrace() const { return '}'; }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_braces_inside_raw_string_are_ignored(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::string_view json() const { return R\"({ \"key\": {} })\"; }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])


    def test_comment_only_body_is_not_reported(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    void noop()\n"
            "    {\n"
            "        // nothing to do { here }\n"
            "        /* still nothing { here } */\n"
            "    }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_blank_lines_do_not_count_as_effective_lines(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::int32_t value() const\n"
            "    {\n"
            "\n"
            "        return value_;\n"
            "\n"
            "    }\n"
            "\n"
            "private:\n"
            "    std::int32_t value_ = 0;\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_digit_separator_does_not_break_literal_parsing(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    std::int32_t value() const { return 1'000'000; }\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_constructor_with_initializer_list_is_reported(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    Sample() : x_(0), y_(1)\n"
            "    {\n"
            "        init();\n"
            "        reset();\n"
            "    }\n"
            "\n"
            "private:\n"
            "    std::int32_t x_;\n"
            "    std::int32_t y_;\n"
            "};\n"
        )
        warnings = check_cppm_inline_functions(file_path)
        self.assertEqual(len(warnings), 1)
        self.assertIn("'Sample'", warnings[0])
        self.assertIn("2 lignes", warnings[0])

    def test_constructor_with_braced_initializers_is_reported(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    Sample() : x_{0}, y_{1} {\n"
            "        init();\n"
            "        reset();\n"
            "    }\n"
            "\n"
            "private:\n"
            "    std::int32_t x_;\n"
            "    std::int32_t y_;\n"
            "};\n"
        )
        warnings = check_cppm_inline_functions(file_path)
        self.assertEqual(len(warnings), 1)
        self.assertIn("'Sample'", warnings[0])

    def test_braced_member_initializers_are_not_functions(self):
        file_path = self.write_cppm(
            "export struct Stats\n"
            "{\n"
            "    std::int32_t count{};\n"
            "    std::array<std::int32_t, 2> values{1, 2};\n"
            "    std::float64_t ratio = 0.0;\n"
            "};\n"
        )
        self.assertEqual(check_cppm_inline_functions(file_path), [])

    def test_multiple_violations_are_reported_in_line_order(self):
        file_path = self.write_cppm(
            "export class Sample\n"
            "{\n"
            "public:\n"
            "    void second()\n"
            "    {\n"
            "        a();\n"
            "        b();\n"
            "    }\n"
            "};\n"
            "\n"
            "export int first()\n"
            "{\n"
            "    int x = 1;\n"
            "    return x;\n"
            "}\n"
        )
        warnings = check_cppm_inline_functions(file_path)
        self.assertEqual(len(warnings), 2)
        self.assertIn("'second'", warnings[0])
        self.assertIn("'first'", warnings[1])

    def test_code_based_entry_point_matches_file_based_one(self):
        body = (
            "export int compute()\n"
            "{\n"
            "    int a = 1;\n"
            "    return a;\n"
            "}\n"
        )
        file_path = self.write_cppm(body)
        violations = check_cppm_inline_function_bodies(FILE_HEADER + body)
        self.assertEqual(len(violations), 1)
        function_name, start_line, body_lines = violations[0]
        self.assertEqual(function_name, "compute")
        self.assertEqual(body_lines, 2)
        self.assertEqual(len(check_cppm_inline_functions(file_path)), 1)
        self.assertIn(f":{start_line} :", check_cppm_inline_functions(file_path)[0])

    def test_message_format_matches_specification(self):
        message = format_cppm_inline_function_message("Src/App/Application.cppm", "Run", 42, 3)
        self.assertEqual(
            message,
            "[WARN_CPPM_INLINE_FUNCTION] Src/App/Application.cppm:42 : "
            "la fonction 'Run' possède un corps de 3 lignes effectives "
            "dans l'interface de module (max : 1). "
            "Déplace l'implémentation dans un fichier .cpp.",
        )


if __name__ == "__main__":
    unittest.main()

