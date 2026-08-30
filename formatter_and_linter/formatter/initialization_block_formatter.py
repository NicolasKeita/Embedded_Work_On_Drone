#!/usr/bin/env python3
"""
Initialization Block Formatter

Enforces an empty line after the first block of local variable declarations
at the beginning of a function body.
"""

import re

DECLARATION_PATTERN = re.compile(
    r"^\s*"
    r"(?:const\s+|constexpr\s+|static\s+|volatile\s+|inline\s+)?"
    r"(?!(?:return|if|for|while|switch|else|catch|goto|throw|co_return|co_yield|co_await|case|default|delete|new|break|continue|using|typedef)\b)"
    r"(?:struct\s+|class\s+|enum\s+)?"
    r"(?:(?:unsigned|signed|long|short)\s+)*"
    r"(?:[a-zA-Z_][a-zA-Z0-9_]*::)*[a-zA-Z_][a-zA-Z0-9_]*(?:<[^;{}]*>)?(?:\s*::\s*[a-zA-Z_][a-zA-Z0-9_]*(?:<[^;{}]*>)?)*" # Type
    r"(?:\s+|\s*\*+\s*|\s*\&+\s*|\s*const\s+|\s*volatile\s*)+" # Separator
    r"(?:[a-zA-Z_][a-zA-Z0-9_]*|\[\s*[a-zA-Z_][a-zA-Z0-9_]*(?:\s*,\s*[a-zA-Z_][a-zA-Z0-9_]*)*\s*\])" # Variable name
    r"\s*(?:=|\(|{|;|,)" # End
)

def format_initialization_blocks(code: str) -> str:
    """
    Inserts an empty line after the first block of declarations in a function body.
    """
    lines = code.splitlines()
    result = []
    
    in_function = False
    brace_depth = 0
    
    in_init_block = False
    has_declarations = False
    
    comment_buffer = []
    
    def flush_comments():
        nonlocal comment_buffer
        if comment_buffer:
            result.extend(comment_buffer)
            comment_buffer = []

    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        
        if not in_function:
            result.append(line)
            if stripped == '{':
                # Check if it's a function by looking at previous non-empty lines
                prev_idx = i - 1
                is_class_or_namespace = False
                while prev_idx >= 0:
                    prev_line = lines[prev_idx].strip()
                    if prev_line:
                        if re.search(r'\b(class|struct|enum|namespace)\b', prev_line):
                            is_class_or_namespace = True
                        break
                    prev_idx -= 1
                
                if not is_class_or_namespace:
                    in_function = True
                    brace_depth = 1
                    in_init_block = True
                    has_declarations = False
            i += 1
            continue
            
        # We are inside a function
        
        # Handle comments
        if stripped.startswith('//'):
            comment_buffer.append(line)
            i += 1
            continue
            
        if stripped.startswith('/*'):
            comment_buffer.append(line)
            if '*/' not in stripped:
                i += 1
                while i < len(lines):
                    comment_buffer.append(lines[i])
                    if '*/' in lines[i]:
                        break
                    i += 1
            i += 1
            continue
            
        # Track brace depth
        brace_depth += stripped.count('{')
        brace_depth -= stripped.count('}')
        
        if brace_depth == 0:
            # End of function
            flush_comments()
            in_function = False
            in_init_block = False
            result.append(line)
            i += 1
            continue
            
        if not in_init_block:
            flush_comments()
            result.append(line)
            i += 1
            continue
            
        # We are in the initialization block
        if not stripped:
            # Empty line
            if has_declarations:
                in_init_block = False
            flush_comments()
            result.append(line)
            i += 1
            continue
            
        is_decl = bool(DECLARATION_PATTERN.match(stripped))
        
        if is_decl:
            has_declarations = True
            flush_comments()
            result.append(line)
            # Skip the rest of the statement
            while i < len(lines) and not (lines[i].strip().endswith(';') or lines[i].strip().endswith('}')):
                i += 1
                if i < len(lines):
                    brace_depth += lines[i].count('{')
                    brace_depth -= lines[i].count('}')
                    result.append(lines[i])
            i += 1
        else:
            # Not a declaration
            if has_declarations:
                result.append('')
            in_init_block = False
            flush_comments()
            result.append(line)
            i += 1

    flush_comments()
    return '\n'.join(result) + ('\n' if code.endswith('\n') else '')

__all__ = ["format_initialization_blocks"]
