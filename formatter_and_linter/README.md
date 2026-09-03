# C++ Code Formatter & Linter

Tools for formatting and linting C++ code.

## Linter

Checks for style violations and code quality issues.

### Features
- **Line length** : Ensures lines don't exceed 120 characters
- **Comment placement** : Comments only above functions
- **Comment language** : Comments must be written in English
- **Function length** : Functions must not exceed 40 lines
- **File length** : 120 lines max per file
- **Multiple variable declarations** : `[MULTIPLE_VAR_DECL] Declare only one variable per line.` Commas inside templates `<>`, call arguments `()` and braced initializers `{}` are ignored

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
- Line joining: merges statements wrapped over several lines back onto a single
  line when the combined line fits within 120 characters.
- Member variable alignment (.cppm only): inside struct / class bodies of
  C++20 module interface files, variable names of contiguous member
  declaration blocks are aligned on one column. The column is computed from
  the longest type of the block plus one space; blank lines, methods, macros
  and visibility changes start a new block; trailing comments are preserved.

**Before :**
```cpp
struct SilConfig {
    double dt = 0.01;
    sim::control::TargetState target{.z = 10.0};
};
```

**After :**
```cpp
struct SilConfig {
    double                    dt = 0.01;
    sim::control::TargetState target{.z = 10.0};
};
```

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