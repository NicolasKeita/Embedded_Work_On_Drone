#!/usr/bin/env python3
"""
Unit tests for the CMakeLists.txt length check rule.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.cmake_checks import (
    MAX_CMAKELISTS_LINES,
    check_cmake_file_lengths,
    find_cmake_files,
)


class TestFindCmakeFiles(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = self.temp_dir.name
        self.original_cwd = os.getcwd()
        os.chdir(self.root)

    def tearDown(self):
        os.chdir(self.original_cwd)
        self.temp_dir.cleanup()

    def create_cmake_file(self, directory: str, line_count: int) -> str:
        os.makedirs(directory, exist_ok=True)
        file_path = os.path.join(directory, "CMakeLists.txt")
        with open(file_path, "w", encoding="utf-8") as handle:
            for i in range(line_count):
                handle.write(f"# line {i}\n")
        return file_path

    def test_root_cmake_file_is_found(self):
        self.create_cmake_file(self.root, 1)
        self.assertEqual(find_cmake_files(["Src"]), ["CMakeLists.txt"])

    def test_nested_cmake_files_are_found(self):
        self.create_cmake_file(os.path.join(self.root, "Src", "Control"), 1)
        self.create_cmake_file(os.path.join(self.root, "Tests", "Utils"), 1)
        found = find_cmake_files(["Src", "Tests"])
        self.assertEqual(len(found), 2)

    def test_missing_cmake_files_are_not_found(self):
        os.makedirs(os.path.join(self.root, "Src"), exist_ok=True)
        self.assertEqual(find_cmake_files(["Src"]), [])


class TestCheckCmakeFileLengths(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = self.temp_dir.name
        self.original_cwd = os.getcwd()
        os.chdir(self.root)

    def tearDown(self):
        os.chdir(self.original_cwd)
        self.temp_dir.cleanup()

    def create_cmake_file(self, directory: str, line_count: int) -> str:
        os.makedirs(directory, exist_ok=True)
        file_path = os.path.join(directory, "CMakeLists.txt")
        with open(file_path, "w", encoding="utf-8") as handle:
            for i in range(line_count):
                handle.write(f"# line {i}\n")
        return file_path

    def test_max_cmakelists_lines_constant(self):
        self.assertEqual(MAX_CMAKELISTS_LINES, 300)

    def test_file_at_limit_is_valid(self):
        self.create_cmake_file(self.root, MAX_CMAKELISTS_LINES)
        self.assertEqual(check_cmake_file_lengths(["Src"]), [])

    def test_file_above_limit_is_reported(self):
        self.create_cmake_file(self.root, MAX_CMAKELISTS_LINES + 1)
        violations = check_cmake_file_lengths(["Src"])
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][0], "CMakeLists.txt")
        self.assertEqual(violations[0][1], MAX_CMAKELISTS_LINES + 1)

    def test_nested_file_above_limit_is_reported(self):
        directory = os.path.join(self.root, "Src", "Big")
        self.create_cmake_file(directory, MAX_CMAKELISTS_LINES + 10)
        violations = check_cmake_file_lengths(["Src"])
        self.assertEqual(len(violations), 1)
        self.assertEqual(
            violations[0][0],
            os.path.join("Src", "Big", "CMakeLists.txt"),
        )

    def test_files_at_and_below_limit_are_not_reported(self):
        self.create_cmake_file(self.root, MAX_CMAKELISTS_LINES)
        self.create_cmake_file(os.path.join(self.root, "Src"), 42)
        self.assertEqual(check_cmake_file_lengths(["Src"]), [])

    def test_custom_threshold_is_applied(self):
        self.create_cmake_file(self.root, 15)
        violations = check_cmake_file_lengths(["Src"], max_lines=10)
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][1], 15)


if __name__ == "__main__":
    unittest.main()
