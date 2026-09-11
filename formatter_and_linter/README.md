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
- **Inline function bodies in .cppm** : `[WARN_CPPM_INLINE_FUNCTION] <path>:<line> : la fonction '<name>' possède un corps de N lignes effectives dans l'interface de module (max : 1). Déplace l'implémentation dans un fichier .cpp.` Module interface files (.cppm) only tolerate single-line function bodies (getters/setters); comments and string/char/raw-string literals are blanked out before brace matching (braces inside them are ignored), struct/class/union/enum/namespace blocks are never mistaken for functions, and only non-empty lines inside the braces count as effective lines
- **Function parameter count** : `[WARN_FUNCTION_TOO_MANY_PARAMETERS] <path>:<line> : la fonction '<name>' possède N paramètres (max : 5). Envisagez de regrouper certains paramètres dans une structure.` Function definitions (with a body) in both .cpp and .cppm files with more than 5 parameters are reported; five is allowed, six or more is not. Only top-level commas in the parameter list are counted, so commas nested inside parentheses (function-pointer parameters), angle brackets (template arguments), square brackets (array parameters) and braces (braced default values) are ignored; function declarations (prototypes ending with `;`) and function calls are not affected

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
- Declaration block compaction: blank lines located *between* consecutive
  variable declarations at the very beginning of a function body are removed
  so the leading declarations are packed together. The zone starts right after
  the opening brace `{` and ends at the first line that is not a declaration
  (control structure, function call, reassignment, ...); the blank line that
  separates the declaration block from the rest of the body is preserved
  (and, when missing, inserted by the existing initialization-block rule).
- Local variable alignment: at the very beginning of every function body, the
  first contiguous block of variable declarations (right after the opening
  brace `{`) is realigned so that all variable names start on the same column,
  computed from the longest declaration type of the block plus one space. The
  rule applies only when the block holds at least two declarations; the
  initializer that follows each name (`=`, `{...}`, `(...)`) is preserved, as
  is the base indentation. It runs after line joining so wrapped declarations
  are already back on a single line, and only for non module-interface files.
- Designated initializer splitting: single-line declarations using a C++20
  designated initializer (`.field = value`) whose line exceeds 120 characters
  are split over several lines. The declaration and its opening brace stay on
  the original line (so the type / variable-name column alignment of the
  surrounding declarations is preserved), every field moves to its own line
  (indented 4 spaces past the variable-name column), the closing `};` is
  aligned with the variable-name column and a trailing comma is added after
  the last field. Only top-level commas are split points, so commas inside
  function calls, template arguments (`<...>`) or nested braced
  sub-initializers never break a sub-expression, and a trailing line comment
  is kept on the closing brace line. The pass runs after line joining and
  after local variable alignment.


**Before :**
```cpp
void sample()
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);

    std::uniform_real_distribution<std::float64_t> window(2.0, 12.0);

    const std::uint64_t roll = generator() % 6;

    apply(roll);
}
```

**After :**
```cpp
void sample()
{
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> window(2.0, 12.0);
    const std::uint64_t roll = generator() % 6;

    apply(roll);
}
```

**Before :**  (member alignment)
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

**Before :**  (local variable alignment)
```cpp
void generate_faults() {
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);
    const std::uint64_t fault_roll = generator() % 6;
    SimulationResult result{.sil_result = sil_result, .failure_reason = FailureReason::None};
    const std::float64_t dx = sil_result.final_x_m - 0.0;

    apply_fault(fault_roll);
}
```

**After :**
```cpp
void generate_faults() {
    std::uniform_real_distribution<std::float64_t> unit(0.0, 1.0);
    std::uniform_real_distribution<std::float64_t> time_window(2.0, 12.0);
    const std::uint64_t                            fault_roll = generator() % 6;
    SimulationResult                               result{.sil_result = sil_result, .failure_reason = FailureReason::None};
    const std::float64_t                           dx = sil_result.final_x_m - 0.0;

    apply_fault(fault_roll);
}
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