#!/usr/bin/env python3
"""
Module Size Checks

Warns when a module or functional namespace accumulates too many
implementation files, mirroring the single responsibility principle applied
at module scale.

Source files (.cpp / .cppm) are grouped by their filename functional prefix
(e.g. 'SilScenarios-Observability-*' groups as 'SilScenarios-Observability')
when the stem carries several hyphen separated segments, otherwise by their
C++20 module declaration ('module <Name>;' or 'export module <Name>;') or by
the bare stem when no declaration exists. Files nested under a deeper prefix
(e.g. 'SilScenarios-Observability-Telemetry-*') are counted in their parent
group, and only the most specific groups above the threshold are reported so
the same files are never flagged twice.
"""

import os
import re
from typing import Dict, List, Optional, Tuple

from linter.style_checks import _mask_strings_and_comments

MAX_IMPLEMENTATION_FILES_PER_MODULE = 8
SOURCE_EXTENSIONS = ('.cpp', '.cppm')
IMPLEMENTATION_EXTENSION = '.cpp'
SUB_MODULE_PLACEHOLDER = "<SubModule>"

MODULE_TOO_LARGE_MESSAGE_TEMPLATE = (
    "[WARN_MODULE_TOO_LARGE] Le module/namespace '{label}' compte {count} fichiers "
    "d'implémentation (seuil : {threshold}). Pense à le subdiviser en sous-modules "
    "(ex : '{suggestion}')."
)

_MODULE_DECLARATION_RE = re.compile(
    r"^\s*(?:export\s+)?module\s+([A-Za-z_]\w*(?::[A-Za-z_]\w*)*)\s*;"
)


def format_module_too_large_message(
    label: str,
    file_count: int,
    suggestion: str,
    max_files: int = MAX_IMPLEMENTATION_FILES_PER_MODULE,
) -> str:
    return MODULE_TOO_LARGE_MESSAGE_TEMPLATE.format(
        label=label,
        count=file_count,
        threshold=max_files,
        suggestion=suggestion,
    )


def _read_file_text(file_path: str) -> str:
    with open(file_path, "r", encoding="utf-8", errors="replace") as handle:
        return handle.read()


def _extract_module_declaration_name(content: str) -> Optional[str]:
    in_block_comment = False
    for line in content.splitlines():
        masked_line, in_block_comment = _mask_strings_and_comments(line, in_block_comment)
        match = _MODULE_DECLARATION_RE.match(masked_line)
        if match:
            return match.group(1).split(":")[0]
    return None


def _resolve_group_segments(file_path: str, file_name: str) -> List[str]:
    stem = os.path.splitext(file_name)[0]
    segments = stem.split("-")
    if len(segments) >= 2:
        return segments[:-1]
    module_name = _extract_module_declaration_name(_read_file_text(file_path))
    if module_name:
        return [module_name]
    return [stem]


def _build_group_tree(directories: List[str]) -> Dict[str, Dict]:
    tree: Dict[str, Dict] = {"count": 0, "children": {}}
    for directory in directories:
        for root, _, files in os.walk(directory):
            for file_name in sorted(files):
                if not file_name.endswith(SOURCE_EXTENSIONS):
                    continue
                segments = _resolve_group_segments(os.path.join(root, file_name), file_name)
                node = tree
                for segment in segments:
                    node = node["children"].setdefault(segment, {"count": 0, "children": {}})
                if file_name.endswith(IMPLEMENTATION_EXTENSION):
                    node["count"] += 1
    return tree


def _subtree_implementation_count(node: Dict) -> int:
    total = node["count"]
    for child in node["children"].values():
        total += _subtree_implementation_count(child)
    return total


def _make_suggestion_label(label: str, children: Dict[str, Dict]) -> str:
    if children:
        return label + "::" + min(children)
    return label + "::" + SUB_MODULE_PLACEHOLDER


def _collect_exceeding_groups(
    node: Dict,
    segments: List[str],
    max_files: int,
    violations: List[Tuple[str, int, str]],
) -> None:
    total = _subtree_implementation_count(node)
    if total > max_files and not any(
        _subtree_implementation_count(child) > max_files
        for child in node["children"].values()
    ):
        label = "::".join(segments)
        violations.append((
            label,
            total,
            _make_suggestion_label(label, node["children"]),
        ))
        return

    for key, child in node["children"].items():
        _collect_exceeding_groups(child, segments + [key], max_files, violations)


def check_module_implementation_counts(
    directories: List[str],
    max_files: int = MAX_IMPLEMENTATION_FILES_PER_MODULE,
) -> List[Tuple[str, int, str]]:
    """
    Report groups holding more than max_files implementation (.cpp) files as
    (label, implementation_count, suggestion_label) tuples sorted by label.
    """
    tree = _build_group_tree(directories)
    violations: List[Tuple[str, int, str]] = []
    for key, node in tree["children"].items():
        _collect_exceeding_groups(node, [key], max_files, violations)
    return sorted(violations)