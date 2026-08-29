#!/usr/bin/env python3
"""
Comment Language Check

Detects non-English comments in C++ code using the lingua language
detector. The detector is restricted to a small set of languages and
built lazily to keep memory usage low.
"""

import re
from typing import List, Tuple

try:
    from lingua import Language, LanguageDetectorBuilder
    LINGUA_AVAILABLE = True
except ImportError:
    LINGUA_AVAILABLE = False

MIN_COMMENT_LENGTH = 10

SUPPORTED_LANGUAGES = [] if not LINGUA_AVAILABLE else [
    Language.ENGLISH,
    Language.FRENCH,
    Language.SPANISH,
    Language.GERMAN,
    Language.DUTCH,
    Language.PORTUGUESE,
]

_detector = None


def _get_detector():
    global _detector
    if _detector is None:
        _detector = (
            LanguageDetectorBuilder
            .from_languages(*SUPPORTED_LANGUAGES)
            .with_low_accuracy_mode()
            .build()
        )
    return _detector


def _get_string_ranges(code: str) -> List[Tuple[int, int]]:
    ranges = []

    raw_string_pattern = re.compile(r'R"([^()]*)\((.*?)\)\1"', re.DOTALL)
    for match in raw_string_pattern.finditer(code):
        ranges.append((match.start(), match.end()))

    string_pattern = re.compile(r'"(?:[^"\\]|\\.)*"')
    for match in string_pattern.finditer(code):
        ranges.append((match.start(), match.end()))

    return ranges


def _is_in_range(pos: int, ranges: List[Tuple[int, int]]) -> bool:
    return any(start <= pos < end for start, end in ranges)


def _clean_multiline_text(raw_text: str) -> str:
    lines = []
    for line in raw_text.splitlines():
        cleaned = line.strip()
        cleaned = cleaned.lstrip('*').strip()
        if cleaned:
            lines.append(cleaned)
    return ' '.join(lines)


def extract_comment_texts(code: str) -> List[Tuple[int, str]]:
    comments = []
    string_ranges = _get_string_ranges(code)

    block_comment_ranges = []
    block_pattern = re.compile(r'/\*.*?\*/', re.DOTALL)
    for match in block_pattern.finditer(code):
        if _is_in_range(match.start(), string_ranges):
            continue
        line_num = code[:match.start()].count('\n') + 1
        text = _clean_multiline_text(match.group(0)[2:-2])
        if text:
            comments.append((line_num, text))
        block_comment_ranges.append((match.start(), match.end()))

    def is_in_string_or_block(pos: int) -> bool:
        if _is_in_range(pos, string_ranges):
            return True
        return _is_in_range(pos, block_comment_ranges)

    singleline_pattern = re.compile(r'//.*$', re.MULTILINE)
    for match in singleline_pattern.finditer(code):
        if is_in_string_or_block(match.start()):
            continue
        line_num = code[:match.start()].count('\n') + 1
        text = match.group(0)[2:].strip()
        if text:
            comments.append((line_num, text))

    return comments


def _normalize_text(text: str) -> str:
    words = []
    for word in text.split():
        split_word = re.sub(r'(?<=[a-z])(?=[A-Z])', ' ', word)
        words.append(split_word)
    return ' '.join(words)


def _is_checkable(text: str) -> bool:
    letter_count = sum(1 for char in text if char.isalpha())
    return letter_count >= MIN_COMMENT_LENGTH


def check_comment_language(code: str) -> List[Tuple[int, str]]:
    if not LINGUA_AVAILABLE:
        return []

    detector = _get_detector()
    violations = []
    for line_num, text in extract_comment_texts(code):
        if not _is_checkable(text):
            continue
        language = detector.detect_language_of(_normalize_text(text))
        if language is not None and language != Language.ENGLISH:
            violations.append((line_num, language.name.title()))
    return violations


__all__ = [
    "LINGUA_AVAILABLE",
    "MIN_COMMENT_LENGTH",
    "extract_comment_texts",
    "check_comment_language",
]
