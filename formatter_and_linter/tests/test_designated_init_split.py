#!/usr/bin/env python3
"""
Designated Initializer Split Tests

pytest unit tests for the designated-initializer splitting formatting pass.

Run with:
    python -m pytest formatter_and_linter/tests/test_designated_init_split.py -v
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.designated_init_split_formatter import split_long_designated_initializations


def _long_scenario_line():
    return (
        "    const FaultScenario                         scenario{"
        ".start_time = 30.0, .fault_type = FaultType::CommunicationLoss, "
        ".duration_s = 10.0, .is_critical = true};\n"
    )


def _expected_scenario_block():
    return (
        "    const FaultScenario                         scenario{\n"
        "                                                    .start_time = 30.0,\n"
        "                                                    .fault_type = FaultType::CommunicationLoss,\n"
        "                                                    .duration_s = 10.0,\n"
        "                                                    .is_critical = true,\n"
        "                                                };\n"
    )


def test_reference_example_is_split_one_field_per_line():
    code = _long_scenario_line()
    assert split_long_designated_initializations(code) == _expected_scenario_block()


def test_table_alignment_of_neighbors_is_preserved():
    code = (
        "    const SilConfig                             config{.duration_s = 32.0, .trace_level = SilLogLevel::Trace};\n"
        + _long_scenario_line()
        + "    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);\n"
    )
    expected = (
        "    const SilConfig                             config{.duration_s = 32.0, .trace_level = SilLogLevel::Trace};\n"
        + _expected_scenario_block()
        + "    const std::expected<SilRunOutput, SilError> outcome = run_traced(config, scenario);\n"
    )
    assert split_long_designated_initializations(code) == expected


def test_short_line_is_untouched():
    code = "    const SilConfig config{.duration_s = 32.0};\n"
    assert split_long_designated_initializations(code) == code


def test_plain_braced_list_without_designated_init_is_untouched():
    code = "    const std::array<std::int32_t, 12> values{" + "1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28" + "};\n"
    assert len(code.rstrip()) > 120
    assert split_long_designated_initializations(code) == code


def test_call_argument_brace_is_untouched():
    code = "    const auto result = run_mission(controller, aircraft, {.a = 1, .b = 2, .c = 3, .d = 4, .e = 5, .f = 6, .g = 7, .h = 8, .i = 9});\n"
    assert len(code.rstrip()) > 120
    assert split_long_designated_initializations(code) == code


def test_nested_braces_and_calls_are_not_broken():
    code = (
        "    const MissionConfig mission{.target = {.z = 100.0, .yaw = 0.5}, .cb = make_cb(1, 2), "
        ".axis = TrackingAxis::x_axis, .tolerance = 0.5, .name = \"axis-x\", .enabled = true};\n"
    )
    expected = (
        "    const MissionConfig mission{\n"
        "                            .target = {.z = 100.0, .yaw = 0.5},\n"
        "                            .cb = make_cb(1, 2),\n"
        "                            .axis = TrackingAxis::x_axis,\n"
        "                            .tolerance = 0.5,\n"
        "                            .name = \"axis-x\",\n"
        "                            .enabled = true,\n"
        "                        };\n"
    )
    assert len(code.rstrip()) > 120
    assert split_long_designated_initializations(code) == expected


def test_trailing_comma_is_added_after_last_field():
    result = split_long_designated_initializations(_long_scenario_line())
    assert result.splitlines()[-2].endswith(".is_critical = true,")


def test_trailing_comment_is_kept_on_closing_brace():
    code = (
        "    const FaultScenario                         scenario{"
        ".start_time = 30.0, .fault_type = FaultType::CommunicationLoss, "
        ".duration_s = 10.0, .is_critical = true}; // critical path\n"
    )
    expected = _expected_scenario_block().replace("};\n", "}; // critical path\n")
    assert split_long_designated_initializations(code) == expected


def test_designated_with_brace_value_is_split():
    code = (
        "    const MissionConfig mission{.target{.z = 100.0}, .duration = 120.0, "
        ".axis = TrackingAxis::x_axis, .tolerance = 0.5, .enabled = true, .name = \"x\"};\n"
    )
    assert len(code.rstrip()) > 120
    result = split_long_designated_initializations(code)
    assert "                            .target{.z = 100.0},\n" in result


def test_class_and_enum_declarations_are_untouched():
    code = "    enum class VeryLongEnumNameForTestingTheBehaviour{FirstValue = 1, SecondValue = 2, ThirdValue = 3, FourthValue = 4, FifthValue = 5, SixthValue = 6};\n"
    assert len(code.rstrip()) > 120
    assert split_long_designated_initializations(code) == code


def test_strings_and_chars_are_protected_from_splitting():
    code = (
        "    const LabelConfig label{.text = \"a, b\", .sep = ',', .x = 1, .y = 2, .z = 3, "
        ".w = 4, .h = 5, .size = 6, .bold = true};\n"
    )
    assert len(code.rstrip()) > 120
    result = split_long_designated_initializations(code)
    assert '.text = "a, b",' in result
    assert ".sep = ',',\n" in result


def test_pass_is_idempotent():
    once = split_long_designated_initializations(_long_scenario_line())
    twice = split_long_designated_initializations(once)
    assert twice == once


def test_empty_code_is_returned_unchanged():
    assert split_long_designated_initializations("") == ""
