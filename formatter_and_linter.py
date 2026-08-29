#!/usr/bin/env python3
"""
C++ Code Formatter & Linter

Entry point for the formatter_and_linter package.
"""

import sys
import os
from typing import NoReturn


# Add the package directory to Python path
package_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "formatter_and_linter")
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, package_dir)

# Import modules from the package
import formatter.file_handler as file_handler
import shared.comment_utils as comment_utils
import formatter.function_formatter as function_formatter
import formatter.include_formatter as include_formatter
import formatter.comment_function_formatter as comment_function_formatter
import linter.linter as linter
import formatter.short_if_formatter as short_if_formatter
import formatter.brace_formatter as brace_formatter
import formatter.module_formatter as module_formatter


def to_pascal_case(name: str) -> str:
    if name == "main":
        return name

    if any(c.isupper() for c in name[1:]):
        result = ""
        capitalize_next = False
        for i, char in enumerate(name):
            if char == '_':
                capitalize_next = True
            else:
                if capitalize_next:
                    result += char.upper()
                    capitalize_next = False
                else:
                    result += char
        return result

    parts = name.split('_')
    pascal_parts = [part.capitalize() for part in parts]
    return ''.join(pascal_parts)


def rename_files_to_pascal_case(directory: str) -> None:
    for root, dirs, files in os.walk(directory):
        for file in files:
            if file.endswith(('.cpp', '.hpp', '.h')):
                name, ext = os.path.splitext(file)
                if name != "main":
                    pascal_name = to_pascal_case(name)
                    if name != pascal_name:
                        old_path = os.path.join(root, file)
                        new_path = os.path.join(root, pascal_name + ext)
                        os.rename(old_path, new_path)
                        print(f"Renamed: {file} -> {pascal_name + ext}")


def format_file(input_file: str, in_place: bool, check_only: bool) -> bool:
    code = file_handler.read_input_file(input_file)

    is_module_interface = input_file.lower().endswith('.cppm')
    if is_module_interface:
        formatted_code = code
    else:
        code_without_comments, comments = comment_utils.remove_comments(code)

        formatted_code = include_formatter.format_includes(code_without_comments)

        formatted_code = function_formatter.format_function_params(formatted_code)

        formatted_code = comment_utils.restore_comments(formatted_code, comments)

        formatted_code = comment_function_formatter.format_comment_function_spacing(formatted_code)

        formatted_code = short_if_formatter.format_if_statements(formatted_code)

        formatted_code = brace_formatter.format_control_structure_braces(formatted_code)

        formatted_code = brace_formatter.format_function_braces(formatted_code)

        formatted_code = module_formatter.reorder_using_after_prototypes(formatted_code)

        formatted_code = module_formatter.reorder_using_after_import(formatted_code)

        formatted_code = module_formatter.format_import_order(formatted_code)

        formatted_code = module_formatter.format_module_import_spacing(formatted_code)

    has_long_lines = linter.lint_code(formatted_code, file_path=input_file)

    if check_only:
        return has_long_lines

    if in_place:
        output_file = input_file
    else:
        output_file = "output.cpp"

    file_handler.write_output_file(output_file, formatted_code)

    return has_long_lines


def check_directory_structure() -> bool:
    directory_violations = linter.check_directory_file_counts(
        ["Src", "Tests"],
        max_files=linter.MAX_FILES_PER_DIRECTORY,
    )
    linter.print_directory_file_count_warnings(
        directory_violations,
        linter.MAX_FILES_PER_DIRECTORY,
    )
    return len(directory_violations) > 0


def main() -> NoReturn:
    in_place, check_only, recursive, input_files = file_handler.parse_arguments()

    if check_only:
        input_files = file_handler.find_source_files("Src", include_hpp=False, include_cppm=True)
        input_files += file_handler.find_source_files("Tests", include_hpp=False, include_cppm=True)
        if not input_files:
            print("No source files found in Src/ or Tests/", file=sys.stderr)
            sys.exit(1)
        print(f"Checking {len(input_files)} files in Src/ and Tests/...")
    elif recursive:
        input_files = file_handler.find_source_files("Src", include_hpp=False, include_cppm=True)
        input_files += file_handler.find_source_files("Tests", include_hpp=False, include_cppm=True)
        if not input_files:
            print("No .cpp/.cppm files found in Src/ or Tests/", file=sys.stderr)
            sys.exit(1)
        print(f"Found {len(input_files)} .cpp/.cppm files in Src/ and Tests/...")
    elif not input_files:
        print("Usage: python formatter_and_linter.py [-i] [-r/--recursive] <input_file.cpp> ...", file=sys.stderr)
        print("       python formatter_and_linter.py --check", file=sys.stderr)
        print("  -i, --in-place    : Modify the file in place (otherwise create output.cpp)", file=sys.stderr)
        print("  -r, --recursive   : Process all .cpp/.cppm files in Src/ and Tests/ recursively", file=sys.stderr)
        print("  --check           : Check all files in Src/ and Tests/ without modifying", file=sys.stderr)
        sys.exit(1)

    has_any_long_lines = False

    if check_only or recursive:
        has_directory_violations = check_directory_structure()
    else:
        has_directory_violations = False

    if not check_only and not recursive:
        print("Renaming files to PascalCase...")
        rename_files_to_pascal_case("Src")

    for input_file in input_files:
        has_long_lines = format_file(input_file, in_place, check_only)
        if has_long_lines:
            has_any_long_lines = True

    if has_any_long_lines or has_directory_violations:
        print("\n[FAIL] Style issues detected!")
    else:
        print("\n[PASS] All files pass style checks!")

    exit_code = 1 if (has_any_long_lines or has_directory_violations) else 0
    sys.exit(exit_code)


if __name__ == "__main__":
    main()