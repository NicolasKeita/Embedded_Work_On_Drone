#!/usr/bin/env python3
"""
Main Function Filename Tests

Unit tests for the main()-function filename check.
"""

import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from linter.main_filename_checks import check_main_function_filename, MAIN_FILENAME_MESSAGE


def check_file(code_lines, file_path):
    return check_main_function_filename('\n'.join(code_lines), file_path)


class TestMainFunctionFilename(unittest.TestCase):

    def test_main_in_correctly_named_file_is_valid(self):
        code = [
            'int main()',
            '{',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_file(code, 'Src/main.cpp'), [])

    def test_main_in_wrongly_named_file_is_reported(self):
        code = [
            'int main()',
            '{',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_file(code, 'Src/AppMain.cpp'), [1])

    def test_main_with_arguments_in_wrongly_named_file_is_reported(self):
        code = [
            'int main(int argc, char* argv[])',
            '{',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_file(code, 'Tests/Hil/wrong.cpp'), [1])

    def test_qualified_return_type_is_detected(self):
        code = [
            'std::int32_t main()',
            '{',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_file(code, 'Src/wrong.cpp'), [1])

    def test_file_without_main_is_valid(self):
        code = [
            'void helper()',
            '{',
            '    step(1);',
            '}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_main_in_single_line_comment_is_ignored(self):
        code = [
            '// int main()',
            'void helper() {}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_main_in_block_comment_is_ignored(self):
        code = [
            '/*',
            'int main()',
            '*/',
            'void helper() {}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_main_in_string_is_ignored(self):
        code = [
            'std::string text = "int main()";',
            'void helper() {}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_preprocessor_lines_are_ignored(self):
        code = [
            '#define RUN_MAIN int main()',
            'void helper() {}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_calls_to_main_are_not_detected(self):
        code = [
            'void recurse()',
            '{',
            '    foo(main(0));',
            '}',
        ]
        self.assertEqual(check_file(code, 'Src/Helper.cpp'), [])

    def test_missing_file_path_skips_check(self):
        code = [
            'int main()',
            '{',
            '    return 0;',
            '}',
        ]
        self.assertEqual(check_main_function_filename('\n'.join(code), ''), [])

    def test_message_text(self):
        self.assertEqual(MAIN_FILENAME_MESSAGE, "[MAIN_FILENAME] A file defining main() must be named 'main.cpp'.")


if __name__ == "__main__":
    unittest.main()
