#!/usr/bin/env python3
"""
C++ Code Formatter & Linter

Entry point for the formatter_and_linter package.
"""

import sys
import os
import io
import shutil
import threading
import contextlib
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
import formatter.prototype_spacing as prototype_spacing
import formatter.initialization_block_formatter as initialization_block_formatter
import formatter.member_alignment_formatter as member_alignment_formatter
import formatter.declaration_blank_line_formatter as declaration_blank_line_formatter
import formatter.local_variable_alignment_formatter as local_variable_alignment_formatter
import formatter.designated_init_split_formatter as designated_init_split_formatter
from formatter.join_lines import join_lines


class ProgressBar:
    def __init__(self, total: int, width: int = 30, interval_sec: float = 1.0) -> None:
        self.total = max(total, 1)
        self.width = width
        self.interval_sec = interval_sec
        self.done = 0
        self.tick = 0
        self.current_file = ""
        self.stop_event = threading.Event()
        self.lock = threading.RLock()
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.active = False
        self._prev_len = 0
        self._stream = sys.stdout
        self._last_logged_done = -1

    def start(self) -> None:
        with self.lock:
            self.active = True
            self._stream = sys.stdout
            self._prev_len = 0
            self._last_logged_done = -1
        self.render()
        self.thread.start()

    def set_file(self, filename: str) -> None:
        with self.lock:
            self.tick = 0
            self.current_file = filename
        self.render()

    def advance(self) -> None:
        with self.lock:
            self.done += 1
            self.tick = 0
        self.render()

    def run(self) -> None:
        while not self.stop_event.wait(self.interval_sec):
            with self.lock:
                self.tick += 1
            self.render()

    def build_line(self) -> str:
        with self.lock:
            done = self.done
            tick = self.tick
            current_file = self.current_file
        ratio = min(done / self.total, 1.0)
        filled = int(ratio * self.width)
        bar = "#" * filled + "-" * (self.width - filled)
        percent = ratio * 100.0
        spinner = "|/-\\"[tick % 4]
        return f"[{bar}] {done}/{self.total} ({percent:5.1f}%) {spinner} {current_file}"

    def is_tty_stream(self) -> bool:
        try:
            return bool(self._stream.isatty())
        except (AttributeError, ValueError):
            return False

    def emit_output_locked(self, output: str, stream) -> None:
        if not output:
            return
        sys.stderr.write(output)
        if not output.endswith("\n"):
            sys.stderr.write("\n")
        sys.stderr.flush()

    def render_locked(self) -> None:
        line = self.build_line()
        if self.is_tty_stream():
            try:
                columns = shutil.get_terminal_size().columns
            except OSError:
                columns = 120
            if columns > 1:
                line = line[: columns - 1]
            padding = " " * max(0, self._prev_len - len(line))
            self._stream.write("\r" + line + padding + "\x1b[K")
            self._stream.flush()
            self._prev_len = len(line)
            return
        if self.done != self._last_logged_done:
            self._stream.write(line + "\n")
            self._stream.flush()
            self._last_logged_done = self.done

    def render(self) -> None:
        with self.lock:
            if not self.active:
                return
            self.render_locked()

    def clear_locked(self) -> None:
        if not self.is_tty_stream():
            return
        self._stream.write("\r" + " " * self._prev_len + "\r")
        self._stream.flush()
        self._prev_len = 0

    def clear(self) -> None:
        with self.lock:
            if not self.active:
                return
            self.clear_locked()

    def run_with_captured_output(self, func, *args, **kwargs):
        with self.lock:
            stream = self._stream
        buffer = io.StringIO()
        with contextlib.redirect_stderr(buffer):
            result = func(*args, **kwargs)
        output = buffer.getvalue()
        with self.lock:
            if not self.active:
                self.emit_output_locked(output, stream)
                return result
            self.clear_locked()
            self.emit_output_locked(output, stream)
            self.render_locked()
        return result

    def stop(self) -> None:
        self.stop_event.set()
        self.thread.join(timeout=2.0)
        with self.lock:
            if not self.active:
                return
            self.active = False
            line = self.build_line()
            if self.is_tty_stream():
                padding = " " * max(0, self._prev_len - len(line))
                self._stream.write("\r" + line + padding + "\n")
                self._stream.flush()
                self._prev_len = 0
                return
            if self._last_logged_done != self.done:
                self._stream.write(line + "\n")
                self._stream.flush()
                self._last_logged_done = self.done


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
        formatted_code = prototype_spacing.clean_code(code)
        formatted_code = member_alignment_formatter.format_member_alignment_for_file(
            input_file, formatted_code
        )
    else:
        code_without_comments, comments = comment_utils.remove_comments(code)

        formatted_code = include_formatter.format_includes(code_without_comments)

        formatted_code = function_formatter.format_function_params(formatted_code)

        formatted_code = comment_utils.restore_comments(formatted_code, comments)

        formatted_code = comment_function_formatter.format_comment_function_spacing(formatted_code)

        formatted_code = short_if_formatter.format_if_statements(formatted_code)

        formatted_code = brace_formatter.format_control_structure_braces(formatted_code)

        formatted_code = brace_formatter.format_function_braces(formatted_code)

        formatted_code = declaration_blank_line_formatter.remove_blank_lines_between_declarations(formatted_code)

        formatted_code = initialization_block_formatter.format_initialization_blocks(formatted_code)

        formatted_code = module_formatter.reorder_using_after_prototypes(formatted_code)

        formatted_code = module_formatter.reorder_using_after_import(formatted_code)

        formatted_code = module_formatter.format_import_order(formatted_code)

        formatted_code = module_formatter.format_module_import_spacing(formatted_code)

        formatted_code = prototype_spacing.clean_code(formatted_code)

    formatted_code = join_lines(formatted_code)

    if not is_module_interface:
        formatted_code = local_variable_alignment_formatter.align_first_declaration_blocks(formatted_code)

    formatted_code = designated_init_split_formatter.split_long_designated_initializations(formatted_code)

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

    module_filename_violations = linter.check_module_filename_convention(["Src", "Tests"])
    linter.print_module_filename_warnings(module_filename_violations)

    module_size_violations = linter.check_module_implementation_counts(["Src", "Tests"])
    linter.print_module_size_warnings(module_size_violations)

    cmake_length_violations = linter.check_cmake_file_lengths(["Src", "Tests"])
    linter.print_cmake_length_warnings(
        cmake_length_violations,
        linter.MAX_CMAKELISTS_LINES,
    )

    return (
        len(directory_violations) > 0
        or len(module_filename_violations) > 0
        or len(module_size_violations) > 0
        or len(cmake_length_violations) > 0
    )


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
        print("  --check           : Check all files in Src/ and Tests/ and CMakeLists.txt without modifying", file=sys.stderr)
        sys.exit(1)

    has_any_long_lines = False

    if check_only or recursive:
        has_directory_violations = check_directory_structure()
    else:
        has_directory_violations = False

    if not check_only and not recursive:
        print("Renaming files to PascalCase...")
        rename_files_to_pascal_case("Src")

    progress = ProgressBar(total=len(input_files), interval_sec=1.0)
    progress.start()
    try:
        for input_file in input_files:
            progress.set_file(input_file)
            has_long_lines = progress.run_with_captured_output(
                format_file, input_file, in_place, check_only
            )
            progress.advance()
            if has_long_lines:
                has_any_long_lines = True
    finally:
        progress.stop()

    if has_any_long_lines or has_directory_violations:
        print("[FAIL] Style issues detected!")
    else:
        print("[PASS] All files pass style checks!")

    exit_code = 1 if (has_any_long_lines or has_directory_violations) else 0
    sys.exit(exit_code)


if __name__ == "__main__":
    main()