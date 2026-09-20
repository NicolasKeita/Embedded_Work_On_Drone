option(DRONE_ENABLE_CODE_QUALITY "Download the formatter/linter and enable its targets" ON)

if(NOT DRONE_ENABLE_CODE_QUALITY)
    return()
endif()

find_package(Python3 3.9 REQUIRED COMPONENTS Interpreter)
include(FetchContent)

FetchContent_Declare(formatter_and_linter
    GIT_REPOSITORY git@github.com:NicolasKeita/My_Linter_And_Formatter_Cpp.git
    GIT_TAG 7c4293c0261a1a864f51c2558e481f211411ee7b
)
FetchContent_MakeAvailable(formatter_and_linter)

set(FORMATTER_AND_LINTER_SCRIPT
    "${formatter_and_linter_SOURCE_DIR}/formatter_and_linter.py")

add_custom_target(lint
    COMMAND "${Python3_EXECUTABLE}" "${FORMATTER_AND_LINTER_SCRIPT}" --check
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Checking C++ sources with formatter_and_linter"
    USES_TERMINAL
    VERBATIM
)

add_custom_target(format
    COMMAND "${Python3_EXECUTABLE}" "${FORMATTER_AND_LINTER_SCRIPT}" -i -r
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    COMMENT "Formatting C++ sources with formatter_and_linter"
    USES_TERMINAL
    VERBATIM
)
