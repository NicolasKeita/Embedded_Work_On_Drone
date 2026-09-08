cmake_minimum_required(VERSION 4.4)

set(ZEPHYR_TOOLCHAIN_VARIANT gnuarmemb
    CACHE STRING "Zephyr toolchain variant")
set(GNUARMEMB_TOOLCHAIN_PATH /usr
    CACHE PATH "GNU Arm Embedded toolchain root")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")

# GCC 15's installed bare-metal std.cc lacks availability guards that are
# present upstream in GCC 16.  Prepare a checked, build-local copy without
# changing the system toolchain.
include(cmake/PrepareArmStdModule.cmake)

# This initializes module scanning for targets created by Zephyr itself,
# including the `app` target created while find_package(Zephyr) runs.
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(DroneFlightSim VERSION 1.0.0 LANGUAGES CXX)

# Zephyr 3.7 force-configures cross compilers and consequently skips CMake's
# native C++ feature discovery.  Load CMake's GCC module support and run that
# discovery after Zephyr has selected GCC so CMake can expose cxx_std_23 and
# construct its import-std target.
include(Compiler/GNU-CXX)
function(project_discover_cxx_features)
    include(CMakeDetermineCompilerSupport)
    cmake_determine_compiler_support(CXX)

    foreach(feature_variable IN ITEMS
            CMAKE_CXX_COMPILE_FEATURES
            CMAKE_CXX98_COMPILE_FEATURES
            CMAKE_CXX11_COMPILE_FEATURES
            CMAKE_CXX14_COMPILE_FEATURES
            CMAKE_CXX17_COMPILE_FEATURES
            CMAKE_CXX20_COMPILE_FEATURES
            CMAKE_CXX23_COMPILE_FEATURES
            CMAKE_CXX26_COMPILE_FEATURES
            CMAKE_CXX_COMPILER_IMPORT_STD
            CMAKE_CXX_STDLIB_MODULES_JSON)
        set(${feature_variable} "${${feature_variable}}" PARENT_SCOPE)
    endforeach()
endfunction()
project_discover_cxx_features()

# FindTargetTools.cmake sets CMAKE_CXX_COMPILER_FORCED, leaving the feature
# registry empty even after the GCC support module has been loaded.  GCC 15.2
# has native C++23 and import-std support; restore only those CMake metadata.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15.0)
    list(APPEND CMAKE_CXX_COMPILE_FEATURES cxx_std_20 cxx_std_23)
    list(APPEND CMAKE_CXX20_COMPILE_FEATURES cxx_std_20)
    list(APPEND CMAKE_CXX23_COMPILE_FEATURES cxx_std_23)
    set(CMAKE_CXX_COMPILER_IMPORT_STD 23)
endif()

# CMake's synthesized import-std target de-duplicates the repeated `-imacros`
# token used by Zephyr 3.7 and leaves zephyr_stdint.h as a positional input.
# Use GCC's joined spelling for that option so the pair remains indivisible.
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

# Exceptions and RTTI are disabled project-wide: embedded code relies on
# std::expected / std::optional for error handling and value semantics only.
if(MSVC)
    add_compile_options(/EHs- /GR-)
else()
    add_compile_options(-fno-exceptions -fno-rtti)
endif()

# Source lists of every module and test suite (.cppm + .cpp kept together in the same
# variable) live in cmake/Sources.cmake and cmake/HilSources.cmake.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sources)
include(HilSources)

function(target_add_mixed_sources TARGET VISIBILITY BASE_DIR)
    set(ALL_FILES ${ARGN})

    set(INTERFACE_FILES ${ALL_FILES})
    list(FILTER INTERFACE_FILES INCLUDE REGEX "\\.cppm$")

    set(IMPLEMENTATION_FILES ${ALL_FILES})
    list(FILTER IMPLEMENTATION_FILES EXCLUDE REGEX "\\.cppm$")

    if(IMPLEMENTATION_FILES)
        target_sources(${TARGET} ${VISIBILITY} ${IMPLEMENTATION_FILES})

        # Implementation units remain ordinary CXX sources.  Explicitly scan
        # them so `module M;` and `import M;` participate in Ninja's dyndep
        # graph just like the module interface units below.
        set_source_files_properties(${IMPLEMENTATION_FILES}
            TARGET_DIRECTORY ${TARGET}
            PROPERTIES CXX_SCAN_FOR_MODULES ON
        )
    endif()

    if(INTERFACE_FILES)
        target_sources(${TARGET} ${VISIBILITY}
            FILE_SET CXX_MODULES
            BASE_DIRS "${BASE_DIR}"
            FILES ${INTERFACE_FILES}
        )
    endif()
endfunction()

function(project_target_setup target)
    target_compile_features(${target} PRIVATE cxx_std_23)

    set_property(TARGET ${target} PROPERTY CXX_EXTENSIONS OFF)
    set_property(TARGET ${target} PROPERTY CXX_SCAN_FOR_MODULES ON)
    set_property(TARGET ${target} PROPERTY CXX_MODULE_STD ON)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /external:W0 /external:anglebrackets)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

if(TARGET app)
    add_library(flight_firmware_core STATIC)
    target_add_mixed_sources(flight_firmware_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
        ${FIRMWARE_FILES}
    )
    project_target_setup(flight_firmware_core)

    project_target_setup(app)
    target_link_libraries(app PRIVATE flight_firmware_core)
    target_sources(app PRIVATE Src/main.cpp)

    set_source_files_properties(Src/main.cpp
        TARGET_DIRECTORY app
        PROPERTIES CXX_SCAN_FOR_MODULES ON
    )

    return()
endif()

# ----------------------------------------------------------------------------
# flight_sim_core : common static library. The simulation physics, flight control
# law, safety subsystem, SIL orchestration engine, fault-injection vocabulary,
# observability and reporting logic are compiled once here and linked by every
# runner so the three executables share one deterministic engine.
# ----------------------------------------------------------------------------
add_library(flight_sim_core STATIC)
target_add_mixed_sources(flight_sim_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SRC_FILES}
)
project_target_setup(flight_sim_core)

# ----------------------------------------------------------------------------
# SIL_RUNNER : deterministic software-in-the-loop test runner executing at maximum
# CPU speed. With no argument the usage helper is printed; --scenario selects one
# scenario (NOMINAL-001..011, FAULT_INJECTOR-001..005), --all sweeps every scenario;
# -v/--verbose enables per-step logging.
# ----------------------------------------------------------------------------
add_executable(SIL_RUNNER Src/Runners/Sil/main.cpp)
target_add_mixed_sources(SIL_RUNNER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_HARNESS_FILES}
    ${TEST_SIMULATION_FILES}
    ${TEST_SIL_FILES}
    Src/Runners/Sil/SilRunnerCli.cppm
    Src/Runners/Sil/SilRunnerCli.cpp
)
project_target_setup(SIL_RUNNER)
target_link_libraries(SIL_RUNNER PRIVATE flight_sim_core)

# ----------------------------------------------------------------------------
# HIL_RUNNER : hardware-in-the-loop real-time test runner (1 s simulation time =
# 1 s wall-clock time) interfacing with the hardware abstraction layer. --scenario
# selects one mission-level scenario, --interface selects the hardware channel,
# --selftest runs the deterministic HIL validation suite.
# ----------------------------------------------------------------------------
add_executable(HIL_RUNNER Src/Runners/Hil/main.cpp)
target_add_mixed_sources(HIL_RUNNER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_CLI_FILES}
    ${HIL_INFRA_FILES}
    ${HIL_CHANNEL_FILES}
    ${TEST_HARNESS_FILES}
    ${TEST_HIL_FILES}
)
project_target_setup(HIL_RUNNER)
target_link_libraries(HIL_RUNNER PRIVATE flight_sim_core)

# ----------------------------------------------------------------------------
# SIL_MONTE_CARLO : accelerated statistical batch simulation over the SIL engine.
# --seed selects the master RNG seed, --runs the iteration count and --scenario
# optionally restricts the campaign to one scenario template. -v prints the
# perturbed RunInputs of every run; without it the RunInputs are still shown for
# failed runs. --output-csv / --output-json export a per-run report associating
# the drawn inputs (seeds + dispersion) with the measured output metrics.
# ----------------------------------------------------------------------------
add_executable(SIL_MONTE_CARLO Src/Runners/MonteCarlo/main.cpp)
target_add_mixed_sources(SIL_MONTE_CARLO PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_SIMULATION_FILES}
    ${VALIDATION_FILES}
    Src/Runners/MonteCarlo/MonteCarloCampaign.cppm
    Src/Runners/MonteCarlo/MonteCarloCampaign-Cli.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Summary.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Run.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Dispersion.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Export.cpp
)
project_target_setup(SIL_MONTE_CARLO)
target_link_libraries(SIL_MONTE_CARLO PRIVATE flight_sim_core)
