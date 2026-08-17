# C++ Coding Rules

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
