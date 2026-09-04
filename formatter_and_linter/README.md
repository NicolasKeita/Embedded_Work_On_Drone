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
- **Uninitialized declaration** : A variable declared without initialization (`Type var;`) whose first following executable statement (blank lines and comments ignored) assigns one of its members is reported: `Variable 'var' déclarée puis initialisée par assignation membre par membre. Préférer l'initialisation directe ou un designated initializer (C++20).` Any interleaved instruction cancels the detection
- **Empty-brace initialisation** : `Warning [C++20-designated-init]: Préférez l'initialisation désignée 'Type var{.champ = ...};' plutôt qu'une initialisation vide suivie d'une affectation.` A variable declared with an empty-brace value initialisation (`Type var{};` or `Type var {};`) whose first following executable statement (blank lines and comments ignored) assigns one of its members (`var.champ = value;`) is reported; consecutive member assignments (`var.a = 1; var.b = 2;`) are grouped into a single suggestion `Type var{.a = 1, .b = 2};`. Non-empty initialiser lists (`Type var{123};`, `Type var{.a = 1};`), reads of the variable between the declaration and the assignment, and any other interleaved instruction cancel the detection
- **Module size** : `[WARN_MODULE_TOO_LARGE] Le module/namespace '<Name>' compte N fichiers d'implémentation (seuil : 8). Pense à le subdiviser en sous-modules.` Implementation files (.cpp) are grouped by their C++20 module declaration (`module <Name>;`, partition suffixes folded into the base module) whatever their filename functional prefix is; the prefix (ex: `SilScenarios-Observability-*`) is only the fallback for files without any module declaration; when a group exceeds 8 files a warning suggests splitting it into sub-modules

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