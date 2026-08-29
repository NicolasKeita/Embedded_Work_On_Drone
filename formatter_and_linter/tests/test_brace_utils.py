#!/usr/bin/env python3
"""
Brace Utils Tests

pytest unit tests for the shared brace utilities, focusing on the
extract_function_name guard that ignores parentheses inside string literals.

Run with:
    python -m pytest formatter_and_linter/tests/test_brace_utils.py -v
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from shared.brace_utils import extract_function_name


def test_function_name_is_extracted_from_prototype():
    assert extract_function_name("int compute(int value);") == "compute"


def test_qualified_function_name_returns_last_component():
    assert extract_function_name("void FlightController::reset_pids()") == "reset_pids"


def test_constructor_signature_returns_constructor_name():
    assert extract_function_name("SILRunner::RunContext::RunContext(const SilConfig& cfg)") == "RunContext"


def test_initializer_list_column_with_parens_in_string_is_not_a_function():
    line = "    {'a', \"Repos (RPM = 0, servos = 0)\", scenarios::rest}, {'b', \"beta\", go()},"
    assert extract_function_name(line) is None


def test_string_starting_with_parenthesis_is_ignored():
    line = 'auto config = Config{"(name)", value};'
    assert extract_function_name(line) is None


def test_control_statement_is_not_a_function():
    assert extract_function_name("if (condition)") is None
    assert extract_function_name("while (running)") is None


def test_function_braces_pass_keeps_initializer_rows_with_parens_in_strings():
    from formatter.brace_formatter import format_function_braces

    code = (
        "const std::array<ScenarioEntry, 10> ScenarioCatalog::scenarios_{{\n"
        "    {'a', \"Repos (RPM = 0, servos = 0)\", scenarios::rest},\n"
        "    {'b', \"Montee (RPM > hover)\", scenarios::climb},\n"
        "}};\n"
    )
    assert format_function_braces(code) == code
    assert "{'a'," in format_function_braces(code)
    assert "{'b'," in format_function_braces(code)


if __name__ == "__main__":
    import pytest
    pytest.main([__file__, "-v"])