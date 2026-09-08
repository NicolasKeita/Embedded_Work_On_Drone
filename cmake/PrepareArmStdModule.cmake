set(ARM_GLIBCXX_VERSION "15.2.0")
set(ARM_GLIBCXX_SOURCE_DIR "/usr/arm-none-eabi/include/c++/${ARM_GLIBCXX_VERSION}/bits")
set(ARM_GLIBCXX_STD_SOURCE "${ARM_GLIBCXX_SOURCE_DIR}/std.cc")
set(ARM_GLIBCXX_STD_COMPAT_SOURCE "${ARM_GLIBCXX_SOURCE_DIR}/std.compat.cc")
set(ARM_GLIBCXX_EXPECTED_STD_SHA256
    "cdd19c9a4d9f6138d3f17d80f5833376d98e1eef4ae2e7558a443f856f507ac7")
set(ARM_GLIBCXX_MODULE_DIR "${CMAKE_BINARY_DIR}/generated-libstdc++")

if(NOT EXISTS "${ARM_GLIBCXX_STD_SOURCE}" OR
   NOT EXISTS "${ARM_GLIBCXX_STD_COMPAT_SOURCE}")
    message(FATAL_ERROR
        "The supported ARM libstdc++ ${ARM_GLIBCXX_VERSION} module sources were not found")
endif()

file(SHA256 "${ARM_GLIBCXX_STD_SOURCE}" ARM_GLIBCXX_STD_SHA256)
if(NOT ARM_GLIBCXX_STD_SHA256 STREQUAL ARM_GLIBCXX_EXPECTED_STD_SHA256)
    message(FATAL_ERROR
        "The ARM libstdc++ std.cc differs from the validated GCC 15.2 source")
endif()

find_program(GIT_EXECUTABLE git REQUIRED)
file(MAKE_DIRECTORY "${ARM_GLIBCXX_MODULE_DIR}")
file(COPY_FILE
    "${ARM_GLIBCXX_STD_SOURCE}"
    "${ARM_GLIBCXX_MODULE_DIR}/std.cc"
)
file(COPY_FILE
    "${ARM_GLIBCXX_STD_COMPAT_SOURCE}"
    "${ARM_GLIBCXX_MODULE_DIR}/std.compat.cc"
)
configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/../toolchain/libstdc++/libstdc++.modules.json.in"
    "${ARM_GLIBCXX_MODULE_DIR}/libstdc++.modules.json"
    COPYONLY
)

execute_process(
    COMMAND "${GIT_EXECUTABLE}" apply --unsafe-paths
            "${CMAKE_CURRENT_LIST_DIR}/../toolchain/libstdc++/gcc-15-std-module-bare-metal.patch"
    WORKING_DIRECTORY "${ARM_GLIBCXX_MODULE_DIR}"
    RESULT_VARIABLE ARM_GLIBCXX_PATCH_RESULT
)
if(NOT ARM_GLIBCXX_PATCH_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to apply the validated GCC 16 std module guards")
endif()

set(CMAKE_CXX_STDLIB_MODULES_JSON
    "${ARM_GLIBCXX_MODULE_DIR}/libstdc++.modules.json"
    CACHE FILEPATH "libstdc++ module metadata" FORCE
)
