#!/usr/bin/env python3
"""
Join Lines Tests

pytest unit tests for the line-joining formatting pass.

Run with:
    python -m pytest formatter_and_linter/tests/test_join_lines.py -v
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from formatter.join_lines import join_lines


def test_assignment_broken_after_equal_is_joined():
    code = (
        "    const double total =\n"
        "        value * 2.0;\n"
    )
    expected = "    const double total = value * 2.0;\n"
    assert join_lines(code) == expected


def test_function_arguments_broken_after_comma_are_joined():
    code = (
        "    int result = compute(first_arg,\n"
        "                         second_arg,\n"
        "                         third_arg);\n"
    )
    expected = "    int result = compute(first_arg, second_arg, third_arg);\n"
    assert join_lines(code) == expected


def test_multiline_call_is_joined_by_multi_pass():
    code = (
        "auto verdict = check(value > 0,\n"
        "                    value < 100,\n"
        "                    extra_condition);\n"
    )
    assert join_lines(code) == "auto verdict = check(value > 0, value < 100, extra_condition);\n"


def test_operator_at_start_of_next_line_is_joined():
    code = (
        "    int total = a\n"
        "        + b\n"
        "        - c;\n"
    )
    assert join_lines(code) == "    int total = a + b - c;\n"


def test_stream_output_continuation_is_not_joined():
    code = (
        '    std::cout << "value: "\n'
        "              << value << std::endl;\n"
    )
    assert join_lines(code) == code


def test_stream_output_with_trailing_operator_is_not_joined():
    code = (
        '    std::cout << "t = " << timeSeconds <<\n'
        '              "s   z = " << state.z << std::endl;\n'
    )
    assert join_lines(code) == code


def test_stream_output_block_stays_split():
    code = (
        '    std::cout << "t = " << std::fixed << std::setprecision(kTimePrecision) << timeSeconds\n'
        '              << "s   z = " << std::setprecision(kValuePrecision) << state.z\n'
        '              << "m/s   pitch = " << pitchDegrees\n'
        '              << "deg   rpm = " << state.actual_rpm << std::endl;\n'
    )
    assert join_lines(code) == code


def test_multiline_function_signature_params_are_kept():
    code = (
        "TiltTargets FlightController::updatePositionControl(const TargetState&   t,\n"
        "                                                    const AircraftState& a,\n"
        "                                                    double               dt)\n"
        "{\n"
        "    return TiltTargets{};\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_multiline_prototype_params_are_kept():
    code = (
        "    void compute(const Config&        cfg,\n"
        "                  std::vector<double>& out);\n"
    )
    assert join_lines(code) == code


def test_call_after_multiline_signature_is_still_joined():
    code = (
        "TiltTargets FlightController::updatePositionControl(const TargetState&   t,\n"
        "                                                    const AircraftState& a,\n"
        "                                                    double               dt)\n"
        "{\n"
        "    int result = compute(first_arg,\n"
        "                         second_arg,\n"
        "                         third_arg);\n"
        "    return TiltTargets{};\n"
        "}\n"
    )
    expected = (
        "TiltTargets FlightController::updatePositionControl(const TargetState&   t,\n"
        "                                                    const AircraftState& a,\n"
        "                                                    double               dt)\n"
        "{\n"
        "    int result = compute(first_arg, second_arg, third_arg);\n"
        "    return TiltTargets{};\n"
        "}\n"
    )
    assert join_lines(code) == expected


def test_signature_stream_and_call_together_are_handled():
    code = (
        "TiltTargets FlightController::updatePositionControl(const TargetState&   t,\n"
        "                                                    const AircraftState& a,\n"
        "                                                    double               dt)\n"
        "{\n"
        '    std::cout << "t = " << timeSeconds\n'
        "              << \"s   z = \" << state.z << std::endl;\n"
        "    int result = compute(first_arg,\n"
        "                         second_arg);\n"
        "    return TiltTargets{};\n"
        "}\n"
    )
    expected = (
        "TiltTargets FlightController::updatePositionControl(const TargetState&   t,\n"
        "                                                    const AircraftState& a,\n"
        "                                                    double               dt)\n"
        "{\n"
        '    std::cout << "t = " << timeSeconds\n'
        "              << \"s   z = \" << state.z << std::endl;\n"
        "    int result = compute(first_arg, second_arg);\n"
        "    return TiltTargets{};\n"
        "}\n"
    )
    assert join_lines(code) == expected


def test_access_specifier_does_not_swallow_next_line():
    code = (
        "class Application\n"
        "{\n"
        "public:\n"
        "    int Run() const;\n"
        "private:\n"
        "    void Helper();\n"
        "};\n"
    )
    assert join_lines(code) == code


def test_case_and_default_labels_do_not_swallow_next_line():
    code = (
        "    switch (mode) {\n"
        "    case 1:\n"
        "        do_one();\n"
        "        break;\n"
        "    default:\n"
        "        do_default();\n"
        "    }\n"
    )
    assert join_lines(code) == code


def test_enum_declaration_is_preserved():
    code = (
        "enum class MissionState {\n"
        "    TAKEOFF,\n"
        "    CLIMB,\n"
        "    STATION_KEEPING,\n"
        "    COMPLETE\n"
        "};\n"
    )
    assert join_lines(code) == code


def test_enum_with_underlying_type_is_preserved():
    code = (
        "enum class Axis : std::uint8_t {\n"
        "    x_axis,\n"
        "    y_axis,\n"
        "    z_axis\n"
        "};\n"
    )
    assert join_lines(code) == code


def test_enum_with_brace_on_next_line_is_preserved():
    code = (
        "enum class MissionState\n"
        "{\n"
        "    TAKEOFF,\n"
        "    CLIMB\n"
        "};\n"
    )
    assert join_lines(code) == code


def test_consecutive_attribute_prototypes_are_not_joined():
    code = (
        "[[nodiscard]] std::string format_seconds(double value);\n"
        "[[nodiscard]] std::string format_metric(double value);\n"
    )
    assert join_lines(code) == code


def test_attribute_prototype_after_function_body_is_not_joined():
    code = (
        "void helper()\n"
        "{\n"
        "    do_work();\n"
        "}\n"
        "[[nodiscard]] std::string format_metric(double value);\n"
    )
    assert join_lines(code) == code


def test_plain_consecutive_prototypes_are_not_joined():
    code = (
        "void format_seconds(double value);\n"
        "void format_metric(double value);\n"
    )
    assert join_lines(code) == code


def test_constructor_initializer_list_is_kept():
    code = (
        "SILRunner::RunContext::RunContext(const SilConfig& cfg)\n"
        "    : config{cfg},\n"
        "      fc1{cfg.controller},\n"
        "      comms{cfg.seed},\n"
        "      bus{cfg.bus}\n"
        "{\n"
        "    boot(cfg);\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_constructor_initializer_list_with_multiline_signature_is_kept():
    code = (
        "SILRunner::RunContext::RunContext(const SilConfig& cfg,\n"
        "                                  std::uint32_t seed)\n"
        "    : config{cfg},\n"
        "      comms{seed}\n"
        "{\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_single_member_constructor_initializer_is_kept():
    code = (
        "FlightController::FlightController(ControllerConfig config)\n"
        "    : config_(config)\n"
        "{\n"
        "    reset_pids();\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_plain_enum_after_code_is_preserved():
    code = (
        "    int value = 5;\n"
        "    enum Color {\n"
        "        RED,\n"
        "        GREEN,\n"
        "        BLUE\n"
        "    };\n"
    )
    assert join_lines(code) == code


def test_single_logical_condition_in_if_is_joined():
    code = (
        "        if (last_received >= 0.0\n"
        "            && current_time - last_received > config_.heartbeat_timeout_s) {\n"
        "            is_healthy = false;\n"
        "        }\n"
    )
    expected = (
        "        if (last_received >= 0.0 && current_time - last_received > config_.heartbeat_timeout_s) {\n"
        "            is_healthy = false;\n"
        "        }\n"
    )
    assert join_lines(code) == expected


def test_single_logical_condition_in_return_is_joined():
    code = (
        "    return last_received >= 0.0\n"
        "        && current_time - last_received > timeout;\n"
    )
    expected = "    return last_received >= 0.0 && current_time - last_received > timeout;\n"
    assert join_lines(code) == expected


def test_single_logical_or_condition_is_joined():
    code = (
        "    if (state == MissionState::COMPLETE\n"
        "        || state == MissionState::CLIMB) {\n"
        "        proceed();\n"
        "    }\n"
    )
    expected = (
        "    if (state == MissionState::COMPLETE || state == MissionState::CLIMB) {\n"
        "        proceed();\n"
        "    }\n"
    )
    assert join_lines(code) == expected


def test_single_condition_trailing_operator_is_joined():
    code = (
        "    if (a &&\n"
        "        b) {\n"
        "        handle();\n"
        "    }\n"
    )
    expected = (
        "    if (a && b) {\n"
        "        handle();\n"
        "    }\n"
    )
    assert join_lines(code) == expected


def test_multiple_logical_conditions_stay_split():
    code = (
        "    return std::abs(t.z - a.z) <= config_.altitude_tolerance_m\n"
        "        && std::abs(t.x - a.x) <= config_.position_tolerance_m\n"
        "        && std::abs(t.y - a.y) <= config_.position_tolerance_m;\n"
    )
    assert join_lines(code) == code


def test_multiple_logical_conditions_in_if_stay_split():
    code = (
        "        if (altitude_error < kMaxError\n"
        "            && speed_error < kMaxSpeed\n"
        "            && turn_rate < kMaxTurn) {\n"
        "            proceed();\n"
        "        }\n"
    )
    assert join_lines(code) == code


def test_multiple_logical_conditions_after_equal_stay_split():
    code = (
        "    bool healthy =\n"
        "        voltage > kMinVolt\n"
        "        && current < kMaxCurrent\n"
        "        && temperature < kMaxTemp;\n"
    )
    assert join_lines(code) == code


def test_multi_condition_trailing_operator_stays_split():
    code = (
        "    if (a &&\n"
        "        b &&\n"
        "        c) {\n"
        "        handle();\n"
        "    }\n"
    )
    assert join_lines(code) == code


def _merged_pair(total):
    n = "    int value ="
    m = "x" * (total - len(n) - 1)
    return n, m


def test_merged_line_of_119_columns_is_merged():
    n, m = _merged_pair(119)
    code = f"{n}\n    {m}\n"
    out = join_lines(code)
    assert out.splitlines() == [n + " " + m]
    assert len(out.splitlines()[0]) == 119


def test_merged_line_of_exactly_max_length_is_merged():
    n, m = _merged_pair(120)
    code = f"{n}\n    {m}\n"
    out = join_lines(code)
    lines = out.splitlines()
    assert len(lines) == 1
    assert len(lines[0]) == 120


def test_joined_line_of_121_columns_stays_split():
    n, m = _merged_pair(121)
    code = f"{n}\n    {m}\n"
    assert join_lines(code) == code
    assert len(code.splitlines()[0]) == 15


def test_custom_max_length_overrides_default():
    n = "int value ="
    m = "x" * 83 + ";"
    code = f"{n}\n    {m}\n"
    assert len(n) + 1 + len(m) == 96
    assert join_lines(code, max_length=95) == code
    assert join_lines(code) == n + " " + m + "\n"


def test_lines_with_line_comments_are_never_joined():
    code = (
        "    int value =\n"
        "        42; // computed at runtime\n"
    )
    assert join_lines(code) == code


def test_comment_inside_string_is_not_a_comment_marker():
    code = (
        'std::string url =\n'
        '    "http://example.com/path";\n'
    )
    expected = 'std::string url = "http://example.com/path";\n'
    assert join_lines(code) == expected


def test_comment_between_wrapped_lines_blocks_joining():
    code = (
        "    int value =\n"
        "        // see below\n"
        "        42;\n"
    )
    assert join_lines(code) == code


def test_block_comment_header_is_never_joined():
    code = (
        "/*\n"
        "Filename: Src/App/Application.cpp\n"
        "Description: Application entry point.\n"
        "*/\n"
    )
    assert join_lines(code) == code


def test_preprocessor_directives_are_never_joined():
    code = (
        "constexpr int kMax =\n"
        "#if defined(_WIN32)\n"
        "    1000;\n"
        "#endif\n"
    )
    assert join_lines(code) == code


def test_include_directive_at_start_of_second_line_is_protected():
    code = (
        "    const int value =\n"
        "#include <limits>\n"
    )
    assert join_lines(code) == code


def test_function_definition_allman_brace_is_preserved():
    code = (
        "void foo()\n"
        "{\n"
        "    return;\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_empty_function_body_is_preserved():
    code = (
        "void foo()\n"
        "{\n"
        "}\n"
    )
    assert join_lines(code) == code


def test_existing_single_line_control_brace_keeps_its_body():
    code = (
        "    if (cond) {\n"
        "        do_something();\n"
        "    }\n"
    )
    assert join_lines(code) == code


def test_standalone_control_brace_is_joined_to_header():
    code = (
        "    if (condition)\n"
        "    {\n"
        "        return x;\n"
        "    }\n"
    )
    expected = (
        "    if (condition) {\n"
        "        return x;\n"
        "    }\n"
    )
    assert join_lines(code) == expected


def test_lambda_body_is_not_swallowed():
    code = (
        "    auto scale = [](double input) {\n"
        "        return input * 2.0;\n"
        "    };\n"
    )
    assert join_lines(code) == code


def test_else_header_does_not_swallow_its_body():
    code = (
        "    } else {\n"
        "        other();\n"
        "    }\n"
    )
    assert join_lines(code) == code


def test_braced_initializer_list_is_joined():
    code = (
        "std::vector<std::string> names = {\n"
        '    "alice",\n'
        '    "bob",\n'
        '    "carol"};\n'
    )
    expected = 'std::vector<std::string> names = { "alice", "bob", "carol"};\n'
    assert join_lines(code) == expected


def test_designated_initializer_multiline_is_joined():
    code = (
        "    const FaultScenario scenario{\n"
        "        .start_time = 20.0,\n"
        "        .duration = 10.0,\n"
        "        .fault_type = FaultType::SensorFault};\n"
    )
    expected = (
        "    const FaultScenario scenario{ .start_time = 20.0, .duration = 10.0, "
        ".fault_type = FaultType::SensorFault};\n"
    )
    assert join_lines(code) == expected


def test_wrapped_call_above_limit_stays_split():
    code = (
        "    const MissionRunTrace trace =\n"
        "        run_mission(controller, aircraft,\n"
        "                    {.target = {.z = 100.0}, .duration = 90.0,\n"
        "                     .axis = TrackingAxis::z_axis, .tolerance = 2.0});\n"
    )
    out = join_lines(code)
    first = out.splitlines()[0]
    assert first == "    const MissionRunTrace trace = run_mission(controller, aircraft,"
    assert len(out.splitlines()[1]) <= 120
    assert len(out.splitlines()) == 2


def test_semicolon_only_on_next_line_is_joined():
    code = (
        "    int value = compute(a)\n"
        "        ;\n"
    )
    expected = "    int value = compute(a) ;\n"
    assert join_lines(code) == expected


def test_trailing_newline_is_preserved():
    assert join_lines("int a =\n    1;\n") == "int a = 1;\n"
    assert join_lines("int a =\n    1;") == "int a = 1;"
    assert join_lines("int a = 1;") == "int a = 1;"


def test_empty_or_blank_input_is_left_untouched():
    assert join_lines("") == ""
    assert join_lines("\n") == "\n"
    assert join_lines("   \n") == "   \n"


def test_join_lines_is_idempotent():
    source = (
        "    const double total = value * 2.0;\n"
        "    if (condition) {\n"
        "        return x;\n"
        "    }\n"
    )
    assert join_lines(join_lines(source)) == join_lines(source)