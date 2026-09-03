#!/usr/bin/env python3
"""
Member Alignment Tests

Unit tests for the member variable name alignment pass applied to C++20
module interface files (.cppm).
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.member_alignment_formatter import (
    align_member_variables,
    format_member_alignment_for_file,
)


def align_lines(lines, **kwargs):
    return align_member_variables("\n".join(lines) + "\n", **kwargs).splitlines()


def member_line(indent, type_part, name, rest, target):
    return indent + type_part + " " * (target - len(type_part)) + name + rest


class TestMemberAlignment(unittest.TestCase):

    def test_spec_example_block_is_aligned(self):
        code = [
            "struct SilConfig {",
            "    double dt = 0.01;",
            "    sim::control::TargetState target{.z = 10.0};",
            "    sim::control::ControllerConfig controller{.hover_rpm = Aircraft{}.hover_rpm()};",
            "    SensorValidationLimits sensor_limits{};",
            "};",
        ]
        target = len("sim::control::ControllerConfig") + 1
        expected = [
            code[0],
            member_line("    ", "double", "dt", " = 0.01;", target),
            member_line("    ", "sim::control::TargetState", "target", "{.z = 10.0};", target),
            member_line(
                "    ",
                "sim::control::ControllerConfig",
                "controller",
                "{.hover_rpm = Aircraft{}.hover_rpm()};",
                target,
            ),
            member_line("    ", "SensorValidationLimits", "sensor_limits", "{};", target),
            code[5],
        ]
        self.assertEqual(align_lines(code), expected)

    def test_complex_types_are_padded_from_longest_type(self):
        code = [
            "struct Payload {",
            "    const std::uint64_t id = 0;",
            "    std::array<std::uint8_t, 4> checksum{};",
            "    unsigned long counter = 0;",
            "    const AircraftState* state_ptr = nullptr;",
            "};",
        ]
        target = len("std::array<std::uint8_t, 4>") + 1
        expected = [
            code[0],
            member_line("    ", "const std::uint64_t", "id", " = 0;", target),
            member_line("    ", "std::array<std::uint8_t, 4>", "checksum", "{};", target),
            member_line("    ", "unsigned long", "counter", " = 0;", target),
            member_line("    ", "const AircraftState*", "state_ptr", " = nullptr;", target),
            code[5],
        ]
        self.assertEqual(align_lines(code), expected)

    def test_blank_line_starts_new_block(self):
        code = [
            "struct S {",
            "    double alpha = 1.0;",
            "    int beta = 2;",
            "",
            "    long_type_name gamma = 3.0;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    double alpha = 1.0;")
        self.assertEqual(result[2], "    int    beta = 2;")
        self.assertEqual(result[4], "    long_type_name gamma = 3.0;")

    def test_visibility_specifier_resets_block(self):
        code = [
            "class C {",
            "public:",
            "    int a = 1;",
            "    long_name b = 2;",
            "private:",
            "    double c = 3.0;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[0], "class C {")
        self.assertEqual(result[1], "public:")
        self.assertEqual(result[2], "    int       a = 1;")
        self.assertEqual(result[3], "    long_name b = 2;")
        self.assertEqual(result[4], "private:")
        self.assertEqual(result[5], "    double c = 3.0;")
        self.assertEqual(result[6], "};")

    def test_method_breaks_block(self):
        code = [
            "struct S {",
            "    int a = 1;",
            "    long_name b = 2;",
            "    void reset();",
            "    double c = 3.0;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    int       a = 1;")
        self.assertEqual(result[2], "    long_name b = 2;")
        self.assertEqual(result[3], "    void reset();")
        self.assertEqual(result[4], "    double c = 3.0;")

    def test_preprocessor_directive_breaks_block(self):
        code = [
            "struct S {",
            "    int a = 1;",
            "#define LIMIT 10",
            "    long_name b = 2;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    int a = 1;")
        self.assertEqual(result[2], "#define LIMIT 10")
        self.assertEqual(result[3], "    long_name b = 2;")

    def test_trailing_comment_is_preserved(self):
        code = [
            "struct S {",
            "    double alpha = 1.0;   // first",
            "    int beta = 2;  // second",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    double alpha = 1.0; // first")
        self.assertEqual(result[2], "    int    beta = 2; // second")

    def test_comment_line_does_not_break_block(self):
        code = [
            "struct S {",
            "    int a = 1;",
            "    // note about b",
            "    long_name b = 2;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    int       a = 1;")
        self.assertEqual(result[2], "    // note about b")
        self.assertEqual(result[3], "    long_name b = 2;")

    def test_block_comment_does_not_break_block(self):
        code = [
            "struct S {",
            "    int a = 1;",
            "    /*",
            "    Documentation of b.",
            "    */",
            "    long_name b = 2;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    int       a = 1;")
        self.assertEqual(result[2], "    /*")
        self.assertEqual(result[3], "    Documentation of b.")
        self.assertEqual(result[4], "    */")
        self.assertEqual(result[5], "    long_name b = 2;")

    def test_only_cppm_files_are_processed(self):
        code = (
            "struct S {\n"
            "    double alpha = 1.0;\n"
            "    int beta = 2;\n"
            "};\n"
        )
        aligned = (
            "struct S {\n"
            "    double alpha = 1.0;\n"
            "    int    beta = 2;\n"
            "};\n"
        )
        self.assertEqual(format_member_alignment_for_file("Src/Module.cpp", code), code)
        self.assertEqual(format_member_alignment_for_file("Src/Module.hpp", code), code)
        self.assertEqual(format_member_alignment_for_file("Src/Module.h", code), code)
        self.assertEqual(format_member_alignment_for_file("Src/Module", code), code)
        self.assertEqual(format_member_alignment_for_file("Src/Module.cppm", code), aligned)
        self.assertEqual(format_member_alignment_for_file("Src/Module.CPPM", code), aligned)
        self.assertEqual(align_member_variables(code), aligned)

    def test_tab_indentation_is_preserved(self):
        code = "\n".join([
            "struct S {",
            "\tint a = 1;",
            "\tlong_name b = 2;",
            "};",
            "",
        ])
        expected = "\n".join([
            "struct S {",
            "\tint       a = 1;",
            "\tlong_name b = 2;",
            "};",
            "",
        ])
        self.assertEqual(align_member_variables(code), expected)

    def test_nested_struct_blocks_are_independent(self):
        code = [
            "class Outer {",
            "private:",
            "    struct Inner {",
            "        int a = 1;",
            "        long_name b = 2;",
            "        long_name c = 3;",
            "    };",
            "    int d = 4;",
            "    long_name e = 5;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[3], "        int       a = 1;")
        self.assertEqual(result[4], "        long_name b = 2;")
        self.assertEqual(result[5], "        long_name c = 3;")
        self.assertEqual(result[7], "    int       d = 4;")
        self.assertEqual(result[8], "    long_name e = 5;")

    def test_declaration_without_initializer(self):
        code = [
            "struct S {",
            "    std::mutex mutex;",
            "    int a = 1;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    std::mutex mutex;")
        self.assertEqual(result[2], "    int        a = 1;")

    def test_array_member(self):
        code = [
            "struct S {",
            "    std::uint8_t buffer[64];",
            "    int a = 1;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    std::uint8_t buffer[64];")
        self.assertEqual(result[2], "    int          a = 1;")

    def test_template_type_with_parentheses(self):
        code = [
            "struct S {",
            "    std::function<void(int)> callback_;",
            "    std::function<bool(double)> predicate_ = nullptr;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    std::function<void(int)>    callback_;")
        self.assertEqual(result[2], "    std::function<bool(double)> predicate_ = nullptr;")

    def test_pointer_and_reference_styles_are_preserved(self):
        code = [
            "struct S {",
            "    const AircraftState* ptr = nullptr;",
            "    const AircraftState &ref = *state;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    const AircraftState*  ptr = nullptr;")
        self.assertEqual(result[2], "    const AircraftState & ref = *state;")

    def test_string_literal_with_slashes(self):
        code = [
            "struct S {",
            '    std::string endpoint = "http://127.0.0.1:8080";',
            "    int a = 1;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], '    std::string endpoint = "http://127.0.0.1:8080";')
        self.assertEqual(result[2], "    int         a = 1;")

    def test_multiple_declarators_are_untouched(self):
        code = [
            "struct S {",
            "    int a = 1, b = 2;",
            "    long_name c = 3;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[1], "    int a = 1, b = 2;")
        self.assertEqual(result[2], "    long_name c = 3;")

    def test_wrapped_declaration_breaks_block(self):
        code = [
            "struct S {",
            "    double alpha = 1.0;",
            "    long wrapped_value =",
            "        2.0;",
            "    int beta = 3;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result, code)

    def test_block_is_skipped_when_line_would_exceed_limit(self):
        code = [
            "struct S {",
            "    double alpha = 1.0;",
            "    int beta = 22222;",
            "};",
        ]
        result = align_member_variables("\n".join(code) + "\n", max_line_length=20)
        self.assertEqual(result, "\n".join(code) + "\n")

    def test_pass_is_idempotent(self):
        code = (
            "struct S {\n"
            "    double alpha = 1.0;\n"
            "    int beta = 2;\n"
            "    std::uint64_t count = 0;\n"
            "};\n"
        )
        once = align_member_variables(code)
        self.assertEqual(align_member_variables(once), once)

    def test_class_with_methods_and_members(self):
        code = [
            "class CommsBus {",
            "public:",
            "    explicit CommsBus(std::uint64_t seed = 42);",
            "",
            "    void seed(std::uint64_t seed);",
            "",
            "private:",
            "    std::mt19937_64 generator_;",
            "    bool link_up_ = true;",
            "    double loss_probability_ = 0.0;",
            "    std::uint64_t sequence_ = 0;",
            "};",
        ]
        result = align_lines(code)
        target = len("std::mt19937_64") + 1
        self.assertEqual(result[2], "    explicit CommsBus(std::uint64_t seed = 42);")
        self.assertEqual(result[4], "    void seed(std::uint64_t seed);")
        self.assertEqual(result[7], member_line("    ", "std::mt19937_64", "generator_", ";", target))
        self.assertEqual(result[8], member_line("    ", "bool", "link_up_", " = true;", target))
        self.assertEqual(result[9], member_line("    ", "double", "loss_probability_", " = 0.0;", target))
        self.assertEqual(result[10], member_line("    ", "std::uint64_t", "sequence_", " = 0;", target))

    def test_enum_body_is_untouched(self):
        code = [
            "enum class Mode {",
            "    None = 0,",
            "    Auto = 1,",
            "};",
        ]
        self.assertEqual(align_lines(code), code)

    def test_export_struct_brace_on_next_line(self):
        code = [
            "export struct ControlCommand",
            "{",
            "    double wing_rpm = 0.0;",
            "    double left_servo_angle = 0.0;",
            "    int mode = 1;",
            "};",
        ]
        result = align_lines(code)
        self.assertEqual(result[0], "export struct ControlCommand")
        self.assertEqual(result[1], "{")
        self.assertEqual(result[2], "    double wing_rpm = 0.0;")
        self.assertEqual(result[3], "    double left_servo_angle = 0.0;")
        self.assertEqual(result[4], "    int    mode = 1;")
        self.assertEqual(result[5], "};")

    def test_namespace_level_declarations_untouched(self):
        code = [
            "export namespace sim {",
            "    struct S {",
            "        double alpha = 1.0;",
            "        int beta = 2;",
            "    };",
            "    [[nodiscard]] std::string_view name(int value);",
            "}",
        ]
        result = align_lines(code)
        self.assertEqual(result[2], "        double alpha = 1.0;")
        self.assertEqual(result[3], "        int    beta = 2;")
        self.assertEqual(result[5], "    [[nodiscard]] std::string_view name(int value);")


if __name__ == "__main__":
    unittest.main()
