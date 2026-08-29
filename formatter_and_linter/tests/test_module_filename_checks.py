#!/usr/bin/env python3
"""
Unit tests for the module interface / implementation filename check rule.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.module_filename_checks import check_module_filename_convention


class TestCheckModuleFilenameConvention(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = self.temp_dir.name

    def tearDown(self):
        self.temp_dir.cleanup()

    def create_module_files(self, directory: str, file_names) -> None:
        os.makedirs(directory, exist_ok=True)
        for file_name in file_names:
            with open(os.path.join(directory, file_name), "w", encoding="utf-8"):
                pass

    def test_single_exact_implementation_is_valid(self):
        directory = os.path.join(self.root, "Src", "App")
        self.create_module_files(directory, ["Application.cppm", "Application.cpp"])
        self.assertEqual(check_module_filename_convention([directory]), [])

    def test_multiple_hyphenated_implementations_are_valid(self):
        directory = os.path.join(self.root, "Src", "Control")
        self.create_module_files(directory, [
            "FlightController.cppm",
            "FlightController-Core.cpp",
            "FlightController-Loops.cpp",
        ])
        self.assertEqual(check_module_filename_convention([directory]), [])

    def test_missing_implementation_is_reported(self):
        directory = os.path.join(self.root, "Src", "Utils")
        self.create_module_files(directory, ["Logger.cppm"])
        violations = check_module_filename_convention([directory])
        self.assertEqual(len(violations), 1)
        self.assertTrue(violations[0][0].endswith("Logger.cppm"))

    def test_single_hyphenated_implementation_is_reported(self):
        directory = os.path.join(self.root, "Src", "Utils")
        self.create_module_files(directory, ["Logger.cppm", "Logger-Core.cpp"])
        violations = check_module_filename_convention([directory])
        self.assertEqual(len(violations), 1)
        self.assertTrue(violations[0][0].endswith("Logger-Core.cpp"))
        self.assertIn("must be named 'Logger.cpp'", violations[0][1])

    def test_base_implementation_among_multiple_is_reported(self):
        directory = os.path.join(self.root, "Src", "SIL")
        self.create_module_files(directory, [
            "SilRunner.cppm",
            "SilRunner.cpp",
            "SilRunner-Loop.cpp",
        ])
        violations = check_module_filename_convention([directory])
        self.assertEqual(len(violations), 1)
        self.assertTrue(violations[0][0].endswith("SilRunner.cpp"))

    def test_glued_implementation_among_multiple_is_reported(self):
        directory = os.path.join(self.root, "Src", "SIL")
        self.create_module_files(directory, [
            "SilScenarios.cppm",
            "SilScenarios-Core.cpp",
            "SilScenariosSafety.cpp",
        ])
        violations = check_module_filename_convention([directory])
        self.assertEqual(len(violations), 1)
        self.assertTrue(violations[0][0].endswith("SilScenariosSafety.cpp"))
        self.assertIn("SilScenarios-Safety.cpp", violations[0][1])

    def test_unrelated_files_are_ignored(self):
        directory = os.path.join(self.root, "Src", "Root")
        self.create_module_files(directory, ["main.cpp", "TestHarness.cppm", "TestHarness.cpp"])
        self.assertEqual(check_module_filename_convention([directory]), [])

    def test_implementation_must_live_next_to_interface(self):
        interface_dir = os.path.join(self.root, "Src", "App")
        other_dir = os.path.join(self.root, "Src", "Elsewhere")
        self.create_module_files(interface_dir, ["Application.cppm"])
        self.create_module_files(other_dir, ["Application.cpp"])
        violations = check_module_filename_convention([interface_dir])
        self.assertEqual(len(violations), 1)
        self.assertTrue(violations[0][0].endswith("Application.cppm"))

    def test_violations_are_sorted_by_path(self):
        dir_a = os.path.join(self.root, "Src", "Aaa")
        dir_b = os.path.join(self.root, "Src", "Bbb")
        self.create_module_files(dir_a, ["ModuleA.cppm", "ModuleA-Core.cpp"])
        self.create_module_files(dir_b, ["ModuleB.cppm"])
        violations = check_module_filename_convention([dir_b, dir_a])
        self.assertEqual(
            [file_path for file_path, _ in violations],
            [os.path.join(dir_a, "ModuleA-Core.cpp"), os.path.join(dir_b, "ModuleB.cppm")],
        )


if __name__ == "__main__":
    unittest.main()