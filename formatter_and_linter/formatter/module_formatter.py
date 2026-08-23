#!/usr/bin/env python3
"""
Module Import Formatting

Public entry points for C++20 modules import formatting. The implementation
lives in prototype_detection, using_prototype_reorderer,
using_import_reorderer and import_spacing.

Handles spacing between module declarations and imports, reordering of using
statements, and import block formatting.
"""

from formatter.prototype_detection import (
    is_import_or_include_or_module,
    is_function_prototype,
    is_function_prototype_start,
)
from formatter.using_prototype_reorderer import reorder_using_after_prototypes
from formatter.using_import_reorderer import reorder_using_after_import
from formatter.import_spacing import (
    format_import_order,
    format_module_import_spacing,
)

__all__ = [
    "is_import_or_include_or_module",
    "is_function_prototype",
    "is_function_prototype_start",
    "reorder_using_after_prototypes",
    "reorder_using_after_import",
    "format_import_order",
    "format_module_import_spacing",
]
