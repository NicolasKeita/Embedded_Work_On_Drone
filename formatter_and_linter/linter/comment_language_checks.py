#!/usr/bin/env python3
"""
Comment Language Check

Detects non-English comments in C++ code using the lingua language
detector. Comments are cleaned before detection, comments with fewer
than three significant words are accepted by default, and a confidence
margin tolerates technical or mixed-language wording. Short comments
use a relaxed margin because language statistics are weaker on short
texts. The detector is restricted to the team's common languages and
built lazily to keep memory usage low.
"""

import re
from typing import List, Tuple

try:
    from lingua import Language, LanguageDetectorBuilder
    LINGUA_AVAILABLE = True
except ImportError:
    LINGUA_AVAILABLE = False

MIN_SIGNIFICANT_WORDS = 3
CONFIDENCE_MARGIN = 0.15
SHORT_TEXT_MAX_WORDS = 5
SHORT_TEXT_MARGIN = 0.35

SUPPORTED_LANGUAGES = [] if not LINGUA_AVAILABLE else [
    Language.ENGLISH,
    Language.FRENCH,
    Language.SPANISH,
    Language.GERMAN,
]

_COMMENT_SYNTAX_PATTERN = re.compile(r'/\*+|\*+/|//+|\*+')
_DOC_KEYWORD_PATTERN = re.compile(
    r'\b(?:TODO|FIXME|XXX|HACK|NOTE|NOTICE|WIP|'
    r'@brief|@details|@param|@tparam|@return|@returns|@result|'
    r'@note|@warning|@see|@file|@class|@struct|@function|@throws)\b[:]?'
)
_SEPARATOR_CHARS_PATTERN = re.compile(r'[/\\_\-]+')
_CAMEL_CASE_PATTERN = re.compile(r'(?<=[a-z])(?=[A-Z])')
_NON_WORD_PATTERN = re.compile(r'[^A-Za-z0-9\s]+')
_WHITESPACE_PATTERN = re.compile(r'\s+')

_detector = None


def _get_detector():
    global _detector
    if _detector is None:
        _detector = (
            LanguageDetectorBuilder
            .from_languages(*SUPPORTED_LANGUAGES)
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


def _clean_comment_text(comment_text: str) -> str:
    text = _COMMENT_SYNTAX_PATTERN.sub(' ', comment_text)
    text = _DOC_KEYWORD_PATTERN.sub(' ', text)
    text = _SEPARATOR_CHARS_PATTERN.sub(' ', text)
    text = _CAMEL_CASE_PATTERN.sub(' ', text)
    text = _NON_WORD_PATTERN.sub(' ', text)
    return _WHITESPACE_PATTERN.sub(' ', text).strip()


def _count_significant_words(cleaned_text: str) -> int:
    return sum(
        1
        for word in cleaned_text.split()
        if sum(1 for char in word if char.isalpha()) >= 2
    )


def _compute_confidences(cleaned_text: str):
    detector = _get_detector()
    return detector.compute_language_confidence_values(cleaned_text)


def _evaluate_confidence(cleaned_text: str, word_count: int) -> Tuple[bool, str]:
    confidences = _compute_confidences(cleaned_text)
    if not confidences:
        return True, ""
    if word_count <= SHORT_TEXT_MAX_WORDS:
        margin = SHORT_TEXT_MARGIN
    else:
        margin = CONFIDENCE_MARGIN
    top_language = confidences[0].language
    top_score = confidences[0].value
    english_score = next(
        (entry.value for entry in confidences if entry.language == Language.ENGLISH),
        0.0,
    )
    is_valid = (
        top_language == Language.ENGLISH
        or (top_score - english_score) < margin
    )
    return is_valid, top_language.name.title()


def check_comment_language(comment_text: str) -> bool:
    if not LINGUA_AVAILABLE:
        return True

    cleaned_text = _clean_comment_text(comment_text)
    word_count = _count_significant_words(cleaned_text)
    if word_count < MIN_SIGNIFICANT_WORDS:
        return True

    is_valid, _ = _evaluate_confidence(cleaned_text, word_count)
    return is_valid


def check_code_comments_language(code: str) -> List[Tuple[int, str]]:
    if not LINGUA_AVAILABLE:
        return []

    violations = []
    for line_num, comment_text in extract_comment_texts(code):
        cleaned_text = _clean_comment_text(comment_text)
        word_count = _count_significant_words(cleaned_text)
        if word_count < MIN_SIGNIFICANT_WORDS:
            continue
        is_valid, top_language_name = _evaluate_confidence(cleaned_text, word_count)
        if not is_valid:
            violations.append((line_num, top_language_name))
    return violations


__all__ = [
    "LINGUA_AVAILABLE",
    "MIN_SIGNIFICANT_WORDS",
    "CONFIDENCE_MARGIN",
    "SHORT_TEXT_MAX_WORDS",
    "SHORT_TEXT_MARGIN",
    "extract_comment_texts",
    "check_comment_language",
    "check_code_comments_language",
]
