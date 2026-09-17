# Shared Zephyr/C++23 module setup for the two STM32 firmware applications.

set(PROJECT_ROOT "${CMAKE_CURRENT_LIST_DIR}/..")
set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb CACHE STRING "Zephyr toolchain variant")
set(GNUARMEMB_TOOLCHAIN_PATH /usr CACHE PATH "GNU Arm Embedded toolchain root")
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")

include("${PROJECT_ROOT}/cmake/PrepareArmStdModule.cmake")
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(${FIRMWARE_NAME} LANGUAGES CXX)

include(Compiler/GNU-CXX)
function(firmware_discover_cxx_features)
    include(CMakeDetermineCompilerSupport)
    cmake_determine_compiler_support(CXX)

    foreach(feature_variable IN ITEMS
            CMAKE_CXX_COMPILE_FEATURES
            CMAKE_CXX20_COMPILE_FEATURES
            CMAKE_CXX23_COMPILE_FEATURES
            CMAKE_CXX_COMPILER_IMPORT_STD
            CMAKE_CXX_STDLIB_MODULES_JSON)
        set(${feature_variable} "${${feature_variable}}" PARENT_SCOPE)
    endforeach()
endfunction()
firmware_discover_cxx_features()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15.0)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES cxx_std_20 cxx_std_23)
    list(APPEND CMAKE_CXX20_COMPILE_FEATURES cxx_std_20)
    list(APPEND CMAKE_CXX23_COMPILE_FEATURES cxx_std_23)
    set(CMAKE_CXX_COMPILER_IMPORT_STD 23)
endif()

if(TARGET zephyr_interface AND CONFIG_ENFORCE_ZEPHYR_STDINT)
    set(ZEPHYR_STDINT_HEADER
        "${ZEPHYR_BASE}/include/zephyr/toolchain/zephyr_stdint.h")
    set(ZEPHYR_STDINT_SPLIT_OPTION
        "SHELL: $<TARGET_PROPERTY:compiler,imacros> ${ZEPHYR_STDINT_HEADER}")
    get_target_property(ZEPHYR_INTERFACE_COMPILE_OPTIONS
        zephyr_interface INTERFACE_COMPILE_OPTIONS)
    list(REMOVE_ITEM ZEPHYR_INTERFACE_COMPILE_OPTIONS
        "${ZEPHYR_STDINT_SPLIT_OPTION}")
    set_property(TARGET zephyr_interface PROPERTY INTERFACE_COMPILE_OPTIONS
        "${ZEPHYR_INTERFACE_COMPILE_OPTIONS}")
    target_compile_options(zephyr_interface INTERFACE
        "$<TARGET_PROPERTY:compiler,imacros>${ZEPHYR_STDINT_HEADER}")
endif()

include("${PROJECT_ROOT}/cmake/Sources.cmake")
include("${PROJECT_ROOT}/cmake/HilSources.cmake")

function(firmware_add_mixed_sources TARGET BASE_DIR)
    set(ALL_FILES ${ARGN})
    set(INTERFACE_FILES ${ALL_FILES})
    list(FILTER INTERFACE_FILES INCLUDE REGEX "\\.cppm$")
    set(IMPLEMENTATION_FILES ${ALL_FILES})
    list(FILTER IMPLEMENTATION_FILES EXCLUDE REGEX "\\.cppm$")

    if(IMPLEMENTATION_FILES)
        target_sources(${TARGET} PRIVATE ${IMPLEMENTATION_FILES})
        set_source_files_properties(${IMPLEMENTATION_FILES}
            TARGET_DIRECTORY ${TARGET}
            PROPERTIES CXX_SCAN_FOR_MODULES ON)
    endif()
    if(INTERFACE_FILES)
        target_sources(${TARGET} PUBLIC
            FILE_SET CXX_MODULES BASE_DIRS "${BASE_DIR}" FILES ${INTERFACE_FILES})
    endif()
endfunction()

add_library(flight_firmware_core STATIC)
firmware_add_mixed_sources(flight_firmware_core "${PROJECT_ROOT}" ${FIRMWARE_CORE_FILES})
target_link_libraries(flight_firmware_core PUBLIC zephyr_interface)
target_compile_features(flight_firmware_core PRIVATE cxx_std_23)
target_compile_options(flight_firmware_core PRIVATE -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic)
set_target_properties(flight_firmware_core PROPERTIES
    CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON CXX_MODULE_STD ON)

set(ZEPHYR_HEADER_SOURCES
    "${PROJECT_ROOT}/Src/Embedded/InterFc/ZephyrUartInterFcTransport.cppm"
    "${PROJECT_ROOT}/Src/Embedded/InterFc/ZephyrUartInterFcTransport.cpp"
    "${PROJECT_ROOT}/Src/Embedded/Status/StatusLed.cpp"
    "${FIRMWARE_MAIN}"
)
set_source_files_properties(${ZEPHYR_HEADER_SOURCES}
    PROPERTIES COMPILE_OPTIONS -Wno-pedantic)

target_sources(app PRIVATE "${FIRMWARE_MAIN}")
target_compile_features(app PRIVATE cxx_std_23)
target_compile_options(app PRIVATE -fno-exceptions -fno-rtti -Wall -Wextra -Wpedantic)
set_target_properties(app PROPERTIES
    CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES ON CXX_MODULE_STD ON)
set_source_files_properties("${FIRMWARE_MAIN}"
    TARGET_DIRECTORY app PROPERTIES CXX_SCAN_FOR_MODULES ON)
target_link_libraries(app PRIVATE flight_firmware_core)
target_link_options(zephyr_interface INTERFACE
    "LINKER:--no-warn-rwx-segments")

set(FIRMWARE_ARTIFACT_DIR "${PROJECT_ROOT}/artifacts/stm32")
add_custom_target(named_firmware_artifacts ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${FIRMWARE_ARTIFACT_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:zephyr_final>" "${FIRMWARE_ARTIFACT_DIR}/${FIRMWARE_NAME}.elf"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/zephyr/zephyr.hex" "${FIRMWARE_ARTIFACT_DIR}/${FIRMWARE_NAME}.hex"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_BINARY_DIR}/zephyr/zephyr.bin" "${FIRMWARE_ARTIFACT_DIR}/${FIRMWARE_NAME}.bin"
    DEPENDS zephyr_final
)
