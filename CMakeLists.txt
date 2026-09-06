cmake_minimum_required(VERSION 4.1)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")
project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

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

# Validation exercise of the physics (NOM-002..NOM-007) and autonomous mode-change
# (MC-001..MC-004) scenarios (steps 5 and 6).
add_executable(test_simulation Tests/Sim/main.cpp)
target_add_mixed_sources(test_simulation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${TEST_SIMULATION_FILES}
    Tests/Sim/SimCli.cppm
    Tests/Sim/SimCli.cpp
)
set_property(TARGET test_simulation PROPERTY CXX_SCAN_FOR_MODULES ON)

# Deterministic SIL test suite (NOM-001 nominal, FINJ-001..004 fault injection and
# MC-FINJ-001 fault during climb) plus the observability and telemetry suites.
add_executable(test_sil Tests/Sil/main.cpp)
target_add_mixed_sources(test_sil PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
    ${TEST_HARNESS_FILES}
    ${TEST_SIL_FILES}
)

foreach(target test_simulation test_sil)
    set_property(TARGET ${target} PROPERTY CXX_MODULE_STD ON)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /external:W0
            /external:anglebrackets
        )
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()

    target_compile_features(${target} PRIVATE cxx_std_23)
endforeach()

# Statistical Monte-Carlo validation campaign over the SIL engine: the entry
# point Src/SIL/Validation/Campaign/main.cpp launches N deterministic runs and
# exports the results as CSV.
add_executable(monte_carlo_validation Src/SIL/Validation/Campaign/main.cpp)
target_add_mixed_sources(monte_carlo_validation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
    ${VALIDATION_FILES}
)
set_property(TARGET monte_carlo_validation PROPERTY CXX_SCAN_FOR_MODULES ON)
set_property(TARGET monte_carlo_validation PROPERTY CXX_MODULE_STD ON)

if(MSVC)
    target_compile_options(monte_carlo_validation PRIVATE
        /W4
        /external:W0
        /external:anglebrackets
    )
else()
    target_compile_options(monte_carlo_validation PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(monte_carlo_validation PRIVATE cxx_std_23)

# ----------------------------------------------------------------------------
# HIL Hardware-in-the-Loop (step 13+): HAL, binary HIL-Proto v1.0 protocol, host
# simulation mocks, lockstep smoke test, real-time runner and host FC emulator.
# ----------------------------------------------------------------------------
function(hil_target_setup target)
    set_property(TARGET ${target} PROPERTY CXX_SCAN_FOR_MODULES ON)
    set_property(TARGET ${target} PROPERTY CXX_MODULE_STD ON)
    target_compile_features(${target} PRIVATE cxx_std_23)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /external:W0 /external:anglebrackets)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

# Smoke test : one lockstep step (packet codec + transport connectivity).
# (SensorPacket -> parser -> ISensorInput -> placeholder FC -> IActuatorOutput -> ActuatorPacket).
# NOT the mission-level HIL runner; uses a placeholder control law, not the real FC core.
add_executable(hil_step_smoke_test Src/Embedded/main.cpp)
target_add_mixed_sources(hil_step_smoke_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_FILES}
)
hil_target_setup(hil_step_smoke_test)

# HIL runner : real-time closed-loop mission against the FC target.
add_executable(hil_runner Src/Embedded/Hil/Cli/main.cpp)
target_add_mixed_sources(hil_runner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_CLI_FILES}
    ${HIL_INFRA_FILES}
    ${HIL_CHANNEL_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(hil_runner)

# Host FC emulator target : the real FC core driven over stdio frames. NOT the physical STM32.
add_executable(fc1_hil_host Src/Embedded/Hil/FcHost/main.cpp)
target_add_mixed_sources(fc1_hil_host PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_FC_HOST_FILES}
    ${HIL_CLOCK_FILES}
    ${HIL_CONFIG_FILES}
    ${HIL_MODEL_FILES}
    ${SAFETY_FILES}
    ${HIL_CHANNEL_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(fc1_hil_host)

# HIL deterministic test suite (runner, protocol, data-integrity, fault tests).
add_executable(test_hil Tests/Hil/main.cpp)
target_add_mixed_sources(test_hil PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_HARNESS_FILES}
    ${TEST_HIL_FILES}
    ${HIL_INFRA_FILES}
    ${HIL_CHANNEL_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(test_hil)
