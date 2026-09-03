# C++ Coding Rules

## Embedded Systems & Error Handling

This codebase targets embedded software systems. Code must prioritize determinism, layout predictability, and zero-exception overhead.

* **Explicit fixed-width types**: Do not use ambiguous primitive types (`int`, `long`, `float`, `double`) in data structures, hardware buffers, or state logic. Use exact fixed-width types for integers (`std::uint8_t`, `std::int32_t`, `std::uint64_t`) and C++23 floating-point values (`std::float32_t`, `std::float64_t`) directly via `import std;`.
* **No C++ Exceptions**: Exceptions are strictly prohibited (`-fno-exceptions`).
  * Never use `throw`, `try`, or `catch`.
  * For functions or operations that can fail, return `std::expected<T, E>` (or `std::optional<T>`).
* **Memory & Determinism**:
  * Avoid dynamic memory allocation (e.g., heap allocations, `std::vector` resizing) inside time-critical control loops or real-time tasks.
  * Prefer static allocation, value semantics, and compile-time fixed buffers (`std::array`).


## Coding Style

* **One variable per line**: Declare each variable on its own separate line. Never group multiple variable declarations using commas (e.g., `int a, b;` or `bool x = true, y = false;` are strictly prohibited).

## File Header

Every `.cppm` and `.cpp` file must start with:

```cpp
/*
Filename: Src/utils/Logger.cpp
Description: Logger shared state and filesystem helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/
```

`Filename` must match the actual file path. `Description` must describe the file's purpose.

## Comments

* No comments inside function bodies. If code needs explanation, prefer clearer naming, smaller functions, or move the explanation into the documentation block above the function.
* Document functions with a `/* ... */` block placed directly above the function signature.
* Trailing comments on closing braces (e.g. `} // namespace sim::control`) are not used either.


## C++ Modules

* Use C++ modules for all new C++ code.
* Module interfaces **must** use `.cppm`.
* Module implementations **must** use `.cpp`.
* Do not create `.h`, `.hpp`, or `.hxx` files for modules.
* Keep each module's `.cppm` interface and `.cpp` implementation separate.

Example:

```text
Src/App/Application.cppm
Src/App/Application.cpp
```

Large or medium classes may live in a single `.cppm` interface file with their
implementation split across several `.cpp` files.

When split across multiple files, **every** implementation `.cpp` file must include a hyphen (`-`) followed by a suffix describing its responsibility (e.g., `-Core.cpp`). For small classes with a single implementation file, use the base module name without a hyphen.

Example (split implementation):

```text
Src/Control/FlightController.cppm
Src/Control/FlightController-Core.cpp
Src/Control/FlightController-Loops.cpp
Src/Control/FlightController-Mission.cpp

## Imports

* Prefer `import` over `#include`.
* Use `import std;` for the standard library. This is the recommended approach.
* Do not use individual standard-library `#include`s when `import std;` is sufficient.
* Use `#include` only when technically unavoidable.
* Required `#include`s must be placed in the global module fragment.

Import order:

```cpp
import std;
import <system-or-third-party-module>;

import Project.Module;
```

System and third-party imports always come before project module imports.

## CMake

Keep `.cppm` and `.cpp` files together in the same source variable.

```cmake
set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    Src/Utils/Logger.cppm
    Src/Utils/Logger.cpp
)
```

Do not create separate CMake variables for `.cppm` and `.cpp` files.
