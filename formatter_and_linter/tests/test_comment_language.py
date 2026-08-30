#!/usr/bin/env python3
"""
Unit tests for the comment language check rule.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.comment_language_checks import (
    CONFIDENCE_MARGIN,
    MIN_SIGNIFICANT_WORDS,
    SHORT_TEXT_MARGIN,
    SHORT_TEXT_MAX_WORDS,
    LINGUA_AVAILABLE,
    check_comment_language,
    check_code_comments_language,
)


@unittest.skipUnless(LINGUA_AVAILABLE, "lingua-language-detector is not installed")
class TestCheckCommentLanguage(unittest.TestCase):

    def test_spec_english_with_separators_is_valid(self):
        self.assertTrue(check_comment_language("// Contextual helpers / scenarios."))

    def test_spec_todo_comment_is_valid(self):
        self.assertTrue(check_comment_language("// TODO: Fix memory leak in buffer"))

    def test_spec_french_comment_is_invalid(self):
        self.assertFalse(check_comment_language("// Cette fonction calcule la somme"))

    def test_spec_spanish_comment_is_invalid(self):
        self.assertFalse(check_comment_language("// Calcular la suma total"))

    def test_short_comment_is_valid_by_default(self):
        self.assertTrue(check_comment_language("// Fix bug"))
        self.assertTrue(check_comment_language("/* TODO */"))

    def test_empty_or_syntax_only_comment_is_valid(self):
        self.assertTrue(check_comment_language("//"))
        self.assertTrue(check_comment_language("/* */"))
        self.assertTrue(check_comment_language(""))

    def test_docblock_syntax_and_tags_are_stripped(self):
        self.assertTrue(check_comment_language("/// @brief Computes the sum of two values"))
        self.assertTrue(check_comment_language("/** Returns the mission state. */"))

    def test_camel_case_identifiers_do_not_trigger_detection(self):
        self.assertTrue(check_comment_language(
            "/*\nFilename: Src/SIL/ActuatorFaultInjector.cpp\n"
            "Description: Actuator fault injection implementation.\n*/"
        ))

    def test_french_comment_dominated_by_technical_identifiers_is_invalid(self):
        self.assertFalse(check_comment_language(
            "/*\nOrchestrateur SIL : boucle temporelle synchrone a pas constant reliant toute la\n"
            "chaine Aircraft -> FaultInjector -> Sensors/Comms -> FC1/FC2 -> HealthMonitor ->\n"
            "SafetyManager -> Actuators.\n*/"
        ))

    def test_paths_and_underscores_are_split(self):
        self.assertTrue(check_comment_language("// See helpers/scenarios for details"))

    def test_long_french_comment_is_invalid(self):
        self.assertFalse(check_comment_language(
            "// Reinitialise l'etat des PIDs sans toucher aux gains du regulateur"
        ))

    def test_block_comment_in_code_is_reported_with_line_number(self):
        code = (
            "int a(void) {\n"
            "    return 0;\n"
            "}\n"
            "\n"
            "/* Cette fonction calcule la somme de deux nombres */\n"
            "int add(int x, int y) {\n"
            "    return x + y;\n"
            "}\n"
        )
        violations = check_code_comments_language(code)
        self.assertEqual(violations, [(5, "French")])


class TestConstants(unittest.TestCase):

    def test_thresholds(self):
        self.assertEqual(MIN_SIGNIFICANT_WORDS, 3)
        self.assertAlmostEqual(CONFIDENCE_MARGIN, 0.15)
        self.assertEqual(SHORT_TEXT_MAX_WORDS, 5)
        self.assertAlmostEqual(SHORT_TEXT_MARGIN, 0.35)


if __name__ == "__main__":
    unittest.main()
