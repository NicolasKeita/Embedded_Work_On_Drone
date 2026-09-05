#!/usr/bin/env python3
"""
Local Variable Alignment Tests

Unit tests for the first-block local variable alignment pass applied to C++
function bodies.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.local_variable_alignment_formatter import align_first_declaration_blocks


def align(code):
    return align_first_declaration_blocks("\n".join(code) + "\n")


def aligned_line(indent, type_part, name, rest, target):
    return indent + type_part + " " * (target - len(type_part)) + name + rest


class TestLocalVariableAlignment(unittest.TestCase):

    def test_prompt_example_is_aligned(self):
        long_type = "std::uniform_real_distribution<std::float64_t>"
        code = [
            "void generate_faults() {",
            "    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);",
            "    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);",
            "    std::uniform_real_distribution<std::float64_t> duration_window(1.0, 8.0);",
            "    const std::uint64_t fault_roll = generator() % 6;",
            "    SimulationResult result{.sil_result = sil_result, .failure_reason = FailureReason::None};",
            "    const std::float64_t dx = sil_result.final_x_m - 0.0;",
            "",
            "    apply_fault(fault_roll);",
            "}",
        ]
        target = len(long_type) + 1
        expected = [
            "void generate_faults() {",
            aligned_line("    ", long_type, "unit", "(0.0, 1.0);", target),
            aligned_line("    ", long_type, "time_window", "(2.0, 12.0);", target),
            aligned_line("    ", long_type, "duration_window", "(1.0, 8.0);", target),
            aligned_line("    ", "const std::uint64_t", "fault_roll", " = generator() % 6;", target),
            aligned_line("    ", "SimulationResult", "result", "{.sil_result = sil_result, .failure_reason = FailureReason::None};", target),
            aligned_line("    ", "const std::float64_t", "dx", " = sil_result.final_x_m - 0.0;", target),
            "",
            "    apply_fault(fault_roll);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")
        result = align(code).splitlines()
        columns = {line.index(name) for line, name in [
            (result[1], "unit"),
            (result[4], "fault_roll"),
            (result[5], "result"),
            (result[6], "dx"),
        ]}
        self.assertEqual(len(columns), 1)

    def test_only_two_declarations_align(self):
        code = [
            "void f() {",
            "    int a = 1;",
            "    std::uint64_t counter = 0;",
            "",
            "    use(a, counter);",
            "}",
        ]
        target = len("std::uint64_t") + 1
        expected = [
            "void f() {",
            aligned_line("    ", "int", "a", " = 1;", target),
            aligned_line("    ", "std::uint64_t", "counter", " = 0;", target),
            "",
            "    use(a, counter);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_single_declaration_is_unchanged(self):
        code = [
            "void f() {",
            "    std::uint64_t counter = 0;",
            "",
            "    use(counter);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_first_block_only_second_block_left_alone(self):
        code = [
            "void f() {",
            "    int a = 1;",
            "    long_name b = 2;",
            "",
            "    do_something();",
            "",
            "    int c = 3;",
            "    long_name d = 4;",
            "}",
        ]
        result = align(code).splitlines()
        target = len("long_name") + 1
        self.assertEqual(result[1], aligned_line("    ", "int", "a", " = 1;", target))
        self.assertEqual(result[2], aligned_line("    ", "long_name", "b", " = 2;", target))
        self.assertEqual(result[4], "    do_something();")
        self.assertEqual(result[6], "    int c = 3;")
        self.assertEqual(result[7], "    long_name d = 4;")

    def test_blank_line_ends_block(self):
        code = [
            "void f() {",
            "    int a = 1;",
            "    long_name b = 2;",
            "",
            "    int c = 3;",
            "    long_name d = 4;",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "void f() {",
            aligned_line("    ", "int", "a", " = 1;", target),
            aligned_line("    ", "long_name", "b", " = 2;", target),
            "",
            "    int c = 3;",
            "    long_name d = 4;",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_function_call_is_not_aligned(self):
        code = [
            "void f() {",
            "    apply_fault(fault_roll);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_constructor_call_form_is_supported(self):
        long_type = "std::uniform_real_distribution<std::float64_t>"
        code = [
            "void f() {",
            "    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);",
            "    const std::uint64_t roll = generator() % 6;",
            "",
            "    apply(unit, roll);",
            "}",
        ]
        target = len(long_type) + 1
        expected = [
            "void f() {",
            aligned_line("    ", long_type, "unit", "(0.0, 1.0);", target),
            aligned_line("    ", "const std::uint64_t", "roll", " = generator() % 6;", target),
            "",
            "    apply(unit, roll);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_control_structure_body_is_not_aligned(self):
        code = [
            "void f() {",
            "    if (cond) {",
            "        int a = 1;",
            "        long_name b = 2;",
            "    }",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_lambda_declaration_is_aligned_with_peers(self):
        code = [
            "void f() {",
            "    auto reset = [](int x) { return x; };",
            "    int a = 1;",
            "    long_name b = 2;",
            "",
            "    use();",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "void f() {",
            aligned_line("    ", "auto", "reset", " = [](int x) { return x; };", target),
            aligned_line("    ", "int", "a", " = 1;", target),
            aligned_line("    ", "long_name", "b", " = 2;", target),
            "",
            "    use();",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_struct_body_is_not_aligned(self):
        code = [
            "struct S {",
            "    int a = 1;",
            "    long_name b = 2;",
            "};",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_empty_function_body_is_unchanged(self):
        code = [
            "void f() {",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_allman_brace_on_own_line(self):
        code = [
            "void f()",
            "{",
            "    const std::uint64_t a = 1;",
            "    const std::uint64_t bbb = 2;",
            "",
            "    use(a, bbb);",
            "}",
        ]
        target = len("const std::uint64_t") + 1
        expected = [
            "void f()",
            "{",
            aligned_line("    ", "const std::uint64_t", "a", " = 1;", target),
            aligned_line("    ", "const std::uint64_t", "bbb", " = 2;", target),
            "",
            "    use(a, bbb);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_function_inside_namespace_is_aligned_and_indented(self):
        code = [
            "namespace sim {",
            "void generate() {",
            "    int alpha = 1;",
            "    long_name beta = 2;",
            "",
            "    run(alpha, beta);",
            "}",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "namespace sim {",
            "void generate() {",
            aligned_line("    ", "int", "alpha", " = 1;", target),
            aligned_line("    ", "long_name", "beta", " = 2;", target),
            "",
            "    run(alpha, beta);",
            "}",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_leading_blank_after_brace_is_skipped(self):
        code = [
            "void f() {",
            "",
            "    int a = 1;",
            "    long_name b = 2;",
            "",
            "    use(a, b);",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "void f() {",
            "",
            aligned_line("    ", "int", "a", " = 1;", target),
            aligned_line("    ", "long_name", "b", " = 2;", target),
            "",
            "    use(a, b);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_lone_multiline_declaration_is_left_alone(self):
        code = [
            "void f() {",
            "    SomeType x = make(",
            "        1, 2);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(code) + "\n")

    def test_multiline_brace_init_stays_in_same_group(self):
        code = [
            "void MonteCarloRunner::execute_run(std::uint64_t run_id)",
            "{",
            "    Scenario            scenario = generator_.generate_scenario(run_id);",
            "    const SilConfig     config = make_run_config(config_, scenario);",
            "    const FaultScenario fault = scenario.to_fault_scenario();",
            "    const bool          fault_expected = (scenario.fault_type != FaultType::None);",
            "    SimulationResult             result{",
            "                                     .run_id = run_id,",
            "                                     .scenario_seed = scenario.scenario_seed,",
            "                                     .scenario = scenario,",
            "                                 };",
            "    SILRunner                    runner(config);",
            "    std::array<FaultScenario, 1> scenarios{ fault };",
            "    auto                         outcome = runner.run(std::span<const FaultScenario>{ scenarios });",
            "",
            "    execute(runner, scenarios);",
            "}",
        ]
        target = len("std::array<FaultScenario, 1>") + 1
        expected = [
            "void MonteCarloRunner::execute_run(std::uint64_t run_id)",
            "{",
            aligned_line("    ", "Scenario", "scenario", " = generator_.generate_scenario(run_id);", target),
            aligned_line("    ", "const SilConfig", "config", " = make_run_config(config_, scenario);", target),
            aligned_line("    ", "const FaultScenario", "fault", " = scenario.to_fault_scenario();", target),
            aligned_line("    ", "const bool", "fault_expected", " = (scenario.fault_type != FaultType::None);", target),
            "    SimulationResult             result{",
            "                                     .run_id = run_id,",
            "                                     .scenario_seed = scenario.scenario_seed,",
            "                                     .scenario = scenario,",
            "                                 };",
            aligned_line("    ", "SILRunner", "runner", "(config);", target),
            aligned_line("    ", "std::array<FaultScenario, 1>", "scenarios", "{ fault };", target),
            aligned_line("    ", "auto", "outcome", " = runner.run(std::span<const FaultScenario>{ scenarios });", target),
            "",
            "    execute(runner, scenarios);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_multiline_brace_init_column_is_uniform(self):
        code = [
            "void f() {",
            "    int small = 1;",
            "    SimulationResult result{",
            "        .run_id = run_id,",
            "        .scenario = scenario,",
            "    };",
            "    std::array<FaultScenario, 1> scenarios{ fault };",
            "}",
        ]
        result = align(code).splitlines()
        columns = {
            result[1].index("small"),
            result[2].index("result"),
            result[6].index("scenarios"),
        }
        self.assertEqual(len(columns), 1)

    def test_multiline_paren_init_stays_in_same_group(self):
        code = [
            "void f() {",
            "    int small = 1;",
            "    SILRunner runner(",
            "        config,",
            "        telemetry);",
            "    long_name other = 2;",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "void f() {",
            aligned_line("    ", "int", "small", " = 1;", target),
            "    SILRunner runner(",
            "        config,",
            "        telemetry);",
            aligned_line("    ", "long_name", "other", " = 2;", target),
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_multiline_group_is_idempotent(self):
        code = [
            "void f() {",
            "    int small = 1;",
            "    SimulationResult result{",
            "        .run_id = run_id,",
            "    };",
            "    std::array<FaultScenario, 1> scenarios{ fault };",
            "",
            "    use(small, scenarios);",
            "}",
        ]
        once = align(code)
        self.assertEqual(align(once.splitlines()), once)

    def test_control_structure_after_multiline_statement_ends_block(self):
        code = [
            "void f() {",
            "    int small = 1;",
            "    SimulationResult result{",
            "        .run_id = run_id,",
            "    };",
            "    if (small > 0) {",
            "        use(result);",
            "    }",
            "}",
        ]
        target = len("SimulationResult") + 1
        expected = [
            "void f() {",
            aligned_line("    ", "int", "small", " = 1;", target),
            aligned_line("    ", "SimulationResult", "result", "{", target),
            "        .run_id = run_id,",
            "    };",
            "    if (small > 0) {",
            "        use(result);",
            "    }",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_multiple_functions_each_aligned(self):
        code = [
            "void a() {",
            "    int x = 1;",
            "    long_name y = 2;",
            "    use(x, y);",
            "}",
            "",
            "void b() {",
            "    int q = 3;",
            "    long_name r = 4;",
            "    use(q, r);",
            "}",
        ]
        target = len("long_name") + 1
        expected = [
            "void a() {",
            aligned_line("    ", "int", "x", " = 1;", target),
            aligned_line("    ", "long_name", "y", " = 2;", target),
            "    use(x, y);",
            "}",
            "",
            "void b() {",
            aligned_line("    ", "int", "q", " = 3;", target),
            aligned_line("    ", "long_name", "r", " = 4;", target),
            "    use(q, r);",
            "}",
        ]
        self.assertEqual(align(code), "\n".join(expected) + "\n")

    def test_pass_is_idempotent(self):
        code = [
            "void generate_faults() {",
            "    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);",
            "    const std::uint64_t fault_roll = generator() % 6;",
            "    SimulationResult result{.sil_result = sil_result, .failure_reason = FailureReason::None};",
            "",
            "    apply_fault(fault_roll);",
            "}",
        ]
        once = align(code)
        self.assertEqual(align(once.splitlines()), once)

    def test_empty_input(self):
        self.assertEqual(align_first_declaration_blocks(""), "")

    def test_trailing_newline_preserved(self):
        code = "void f() {\n    int a = 1;\n    long b = 2;\n    use(a, b);\n}\n"
        target = len("long") + 1
        expected = (
            "void f() {\n"
            + aligned_line("    ", "int", "a", " = 1;", target)
            + "\n"
            + aligned_line("    ", "long", "b", " = 2;", target)
            + "\n    use(a, b);\n}\n"
        )
        self.assertEqual(align_first_declaration_blocks(code), expected)


if __name__ == "__main__":
    unittest.main()
