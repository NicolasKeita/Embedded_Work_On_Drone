#!/usr/bin/env python3
"""
Unit tests for the directory file count check rule.

Run with:
    python -m unittest discover -s formatter_and_linter/tests -v
"""

import os
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.directory_checks import (
    MAX_FILES_PER_DIRECTORY,
    check_directory_file_counts,
    count_source_files_per_directory,
)


class TestCheckDirectoryFileCounts(unittest.TestCase):

    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = self.temp_dir.name

    def tearDown(self):
        self.temp_dir.cleanup()

    def create_source_files(self, directory: str, count: int) -> None:
        os.makedirs(directory, exist_ok=True)
        for i in range(count):
            extension = ".cpp" if i % 2 == 0 else ".cppm"
            with open(os.path.join(directory, f"File{i}{extension}"), "w", encoding="utf-8"):
                pass

    def test_max_files_per_directory_constant(self):
        self.assertEqual(MAX_FILES_PER_DIRECTORY, 8)

    def test_directory_at_limit_is_valid(self):
        directory = os.path.join(self.root, "Src", "Small")
        self.create_source_files(directory, MAX_FILES_PER_DIRECTORY)
        self.assertEqual(check_directory_file_counts([directory]), [])

    def test_directory_above_limit_is_reported(self):
        directory = os.path.join(self.root, "Src", "Big")
        self.create_source_files(directory, MAX_FILES_PER_DIRECTORY + 1)
        violations = check_directory_file_counts([directory])
        self.assertEqual(len(violations), 1)
        self.assertEqual(violations[0][1], MAX_FILES_PER_DIRECTORY + 1)

    def test_counts_both_cpp_and_cppm(self):
        directory = os.path.join(self.root, "Src", "Mixed")
        self.create_source_files(directory, 4)
        counts = count_source_files_per_directory([directory])
        self.assertEqual(counts[directory], 4)

    def test_ignores_non_source_files(self):
        directory = os.path.join(self.root, "Src", "Docs")
        os.makedirs(directory, exist_ok=True)
        for name in ("Readme.md", "CMakeLists.txt", "notes.log"):
            with open(os.path.join(directory, name), "w", encoding="utf-8"):
                pass
        self.assertEqual(count_source_files_per_directory([directory]), {})

    def test_empty_directories_are_ignored(self):
        directory = os.path.join(self.root, "Src", "Empty")
        os.makedirs(directory, exist_ok=True)
        self.assertEqual(check_directory_file_counts([directory]), [])

    def test_violations_are_sorted_by_path(self):
        big_a = os.path.join(self.root, "Src", "Aaa")
        big_b = os.path.join(self.root, "Src", "Bbb")
        self.create_source_files(big_a, 10)
        self.create_source_files(big_b, 9)
        violations = check_directory_file_counts([big_b, big_a])
        self.assertEqual([directory for directory, _ in violations], [big_a, big_b])


if __name__ == "__main__":
    unittest.main()
