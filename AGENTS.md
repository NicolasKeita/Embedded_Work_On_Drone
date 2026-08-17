# C++ Coding Guidelines

## File Header

Every C++ source file must start with a file header using the following format:

```cpp
/*
Filename: Src/utils/Logger.cpp
Description: Logger shared state and filesystem helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/
```

The `Filename` field must contain the **actual path and filename** of the file.

The `Description` field must briefly describe the purpose of the file.

The copyright notice must use:

```text
Copyright (c) 2026 Nicolas K.
All rights reserved.
```

This header is required for both `.cppm` and `.cpp` files.

Example for a module interface:

```cpp
/*
Filename: Src/App/Application.cppm
Description: Application module interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/
```

Example for its implementation:

```cpp
/*
Filename: Src/App/Application.cpp
Description: Application module implementation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module App.Application;
```

## C++ Modules

This project uses C++20/23 modules.

Every module must separate its interface from its implementation.

* The module interface **must** use the `.cppm` extension.
* The module implementation **must** use the `.cpp` extension.
* Never use `.h`, `.hpp`, or `.hxx` for module interfaces.
* The `.cppm` file contains the module declaration and its public interface.
* The `.cpp` file contains the implementation.

Example:

```text
Src/
└── App/
    ├── Application.cppm
    └── Application.cpp
```

### Module interface

```cpp
/*
Filename: Src/App/Application.cppm
Description: Application module interface.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module App.Application;

export class Application
{
public:
    void Run();
};
```

### Module implementation

```cpp
/*
Filename: Src/App/Application.cpp
Description: Application module implementation.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module App.Application;

import std;

void Application::Run()
{
}
```

## Imports

Prefer `import` over `#include`.

**Do not use `#include` unless it is strictly required.**

For the standard library, use:

```cpp
import std;
```

`import std;` provides the standard library as a whole and is the **recommended approach in this project**.

Do not replace it with individual standard-library headers unless there is a specific technical reason requiring it.

For example, do not write:

```cpp
#include <vector>
#include <string>
#include <memory>
#include <filesystem>
```

when the required functionality is available through:

```cpp
import std;
```

## Import Order

Imports must always be ordered into two groups:

1. System and third-party imports
2. Project module imports

System and third-party imports must appear before project modules.

Correct:

```cpp
import std;
import spdlog;

import Utils.Logger;
import App.Application;
```

Incorrect:

```cpp
import Utils.Logger;

import std;
import spdlog;
```

## Includes

`#include` is allowed only when it is technically required and cannot reasonably be replaced by `import`.

When an include is required by a module, place it in the **global module fragment**:

```cpp
module;

#include <windows.h>
#include <spdlog/spdlog.h>

module Utils.Logger;

import std;
```

Do not put unnecessary includes after the module declaration.

Do not use `#include` as a substitute for a module import.

## Module Naming

Module names should reflect the source directory and component name.

For example:

```text
Src/Utils/Logger.cppm
```

must define:

```cpp
export module Utils.Logger;
```

Its implementation:

```text
Src/Utils/Logger.cpp
```

must begin with:

```cpp
module Utils.Logger;
```

## CMake

When defining source files in `CMakeLists.txt`, `.cppm` and `.cpp` files must be kept together in the same source-file variable.

Correct:

```cmake
set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    Src/Utils/Logger.cppm
    Src/Utils/Logger.cpp
)
```

Do **not** create separate variables for module interfaces and implementations.

Incorrect:

```cmake
set(MODULE_FILES
    Src/App/Application.cppm
    Src/Utils/Logger.cppm
)

set(SOURCE_FILES
    Src/App/Application.cpp
    Src/Utils/Logger.cpp
)
```

The `.cppm` interface and corresponding `.cpp` implementation should be listed together.

## General Rules

When creating or modifying C++ code:

* Every `.cppm` and `.cpp` file must have the required file header.
* Use `.cppm` for module interfaces.
* Use `.cpp` for module implementations.
* Keep module interfaces and implementations in separate files.
* Prefer `import` over `#include`.
* Use `import std;` for the standard library.
* Only use `#include` when technically unavoidable.
* Put required includes in the global module fragment.
* Put system and third-party imports before project module imports.
* Use project module imports after system and third-party imports.
* Do not introduce traditional header files for new module code unless explicitly required.
* Keep corresponding `.cppm` and `.cpp` files together in CMake source lists.
