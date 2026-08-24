# C++ Code Formatter & Linter

Tools for formatting and linting C++ code.

## Linter

Checks for style violations and code quality issues.

### Features
- **Line length** : Ensures lines don't exceed 120 characters
- **Comment placement** : Comments only above functions
- **Function length** : Functions must not exceed 35 lines
- **File length** : 120 lines max per file

### Examples

**Invalid code :**
```cpp
void function() {
    int x = 5; // Inline comment - FORBIDDEN
}
```

**Valid code :**
```cpp
// Comment above function
void function() {
    int x = 5;
}
```

## Formatter

Automatically formats C++ code according to defined rules.

### Changes
- Function parameter alignment
- Include organization (system vs local)
- If statements on multiple lines
- Function braces on new line
- Proper spacing between functions

### Examples

**Before :**
```cpp
    if (x > 0) return x;
```

**After :**
```cpp
    if (x > 0) {
        return x;
    }
```

## Usage
```bash
python formatter.py file.cpp
python formatter.py -i file.cpp  # in-place modification
```