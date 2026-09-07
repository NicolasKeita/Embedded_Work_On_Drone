cmake_minimum_required(VERSION 4.1)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")
project(DroneFlightSim VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
    set_property(TARGET ${target} PROPERTY CXX_SCAN_FOR_MODULES ON)
    set_property(TARGET ${target} PROPERTY CXX_MODULE_STD ON)
    target_compile_features(${target} PRIVATE cxx_std_23)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /external:W0 /external:anglebrackets)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# ----------------------------------------------------------------------------
# flight_sim_core : common static library. The simulation physics, flight control
# law, safety subsystem, SIL orchestration engine, fault-injection vocabulary,
# observability and reporting logic are compiled once here and linked by every
# runner so the three executables share one deterministic engine.
# ----------------------------------------------------------------------------
add_library(flight_sim_core STATIC)
target_add_mixed_sources(flight_sim_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
)
project_target_setup(flight_sim_core)

# ----------------------------------------------------------------------------
# SIL_RUNNER : deterministic software-in-the-loop test runner executing at maximum
# CPU speed. --scenario selects one scenario (NOMINAL-001..010, FAULT_INJECTOR-001..004);
# -v/--verbose enables per-step logging.
# ----------------------------------------------------------------------------
add_executable(SIL_RUNNER Src/Runners/SilRunnerMain.cpp)
target_add_mixed_sources(SIL_RUNNER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_HARNESS_FILES}
    ${TEST_SIMULATION_FILES}
    ${TEST_SIL_FILES}
)
project_target_setup(SIL_RUNNER)
target_link_libraries(SIL_RUNNER PRIVATE flight_sim_core)

# ----------------------------------------------------------------------------
# HIL_RUNNER : hardware-in-the-loop real-time test runner (1 s simulation time =
# 1 s wall-clock time) interfacing with the hardware abstraction layer. --scenario
# selects one mission-level scenario, --interface selects the hardware channel,
# --selftest runs the deterministic HIL validation suite.
# ----------------------------------------------------------------------------
add_executable(HIL_RUNNER Src/Runners/HilRunnerMain.cpp)
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
# optionally restricts the campaign to one scenario template.
# ----------------------------------------------------------------------------
add_executable(SIL_MONTE_CARLO Src/Runners/MonteCarloMain.cpp)
target_add_mixed_sources(SIL_MONTE_CARLO PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${VALIDATION_FILES}
)
project_target_setup(SIL_MONTE_CARLO)
target_link_libraries(SIL_MONTE_CARLO PRIVATE flight_sim_core)