cmake_minimum_required(VERSION 4.4)

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
project(DroneFlightSim VERSION 1.0.0 LANGUAGES CXX)

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
include(CodeQuality)

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

# ----------------------------------------------------------------------------
# flight_sim_core : common static library. The simulation physics, flight control
# law, safety subsystem, SIL orchestration engine, fault-injection vocabulary,
# observability and reporting logic are compiled once here and linked by every
# runner so the three executables share one deterministic engine.
# ----------------------------------------------------------------------------
add_library(flight_sim_core STATIC)
set(HOST_CONFIG_FILES
    Src/Host/Config/ConfigFile.cppm
    Src/Host/Config/ConfigFile.cpp
    Src/Host/Config/SimulationConfig.cppm
    Src/Host/Config/SimulationConfig.cpp
    Src/Host/Config/ScenarioConfig.cppm
    Src/Host/Config/ScenarioConfig.cpp
    Src/Host/Config/SilRuntimeConfig.cppm
    Src/Host/Config/SilRuntimeConfig.cpp
)
target_add_mixed_sources(flight_sim_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SRC_FILES}
    ${HOST_CONFIG_FILES}
)
project_target_setup(flight_sim_core)

# ----------------------------------------------------------------------------
# SIL_RUNNER : deterministic software-in-the-loop test runner executing at maximum
# CPU speed. With no argument the usage helper is printed; --scenario selects one
# shared scenario (NOMINAL-001, FAULT_INJECTOR-001/003), --all sweeps every scenario;
# -v/--verbose enables per-step logging.
# ----------------------------------------------------------------------------
add_executable(SIL_RUNNER Src/Runners/Sil/main.cpp)
set_target_properties(SIL_RUNNER PROPERTIES OUTPUT_NAME sil_runner)
target_add_mixed_sources(SIL_RUNNER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_HARNESS_FILES}
    ${TEST_SIMULATION_FILES}
    ${TEST_SIL_FILES}
    Src/Runners/Sil/SilRunnerCli.cppm
    Src/Runners/Sil/SilRunnerCli-Parse.cpp
    Src/Runners/Sil/SilRunnerCli-Usage.cpp
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher.cppm
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Sockets.cpp
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Crypto.cpp
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Handshake.cpp
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
set_target_properties(HIL_RUNNER PROPERTIES OUTPUT_NAME hil_runner)
target_add_mixed_sources(HIL_RUNNER PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_CLI_FILES}
    ${HIL_INFRA_FILES}
    ${HIL_CHANNEL_FILES}
    ${TEST_HARNESS_FILES}
    ${TEST_HIL_FILES}
    Src/Host/Config/HilRuntimeConfig.cppm
    Src/Host/Config/HilRuntimeConfig.cpp
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
set_target_properties(SIL_MONTE_CARLO PROPERTIES OUTPUT_NAME sil_monte_carlo)
target_add_mixed_sources(SIL_MONTE_CARLO PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${TEST_SIMULATION_FILES}
    ${VALIDATION_FILES}
    Src/Runners/MonteCarlo/MonteCarloCampaign.cppm
    Src/Runners/MonteCarlo/MonteCarloCampaign-Cli.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Options.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Config.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Summary.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Run.cpp
    Src/Runners/MonteCarlo/MonteCarloCampaign-Dispersion.cpp
    Src/Runners/MonteCarlo/Export/MonteCarloCampaign-Csv.cpp
    Src/Runners/MonteCarlo/Export/MonteCarloCampaign-Json.cpp
)
project_target_setup(SIL_MONTE_CARLO)
target_link_libraries(SIL_MONTE_CARLO PRIVATE flight_sim_core)

set(LINUX_ARTIFACT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/artifacts/linux")
foreach(product IN ITEMS SIL_RUNNER HIL_RUNNER SIL_MONTE_CARLO)
    add_custom_command(TARGET ${product} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${LINUX_ARTIFACT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:${product}>" "${LINUX_ARTIFACT_DIR}/$<TARGET_FILE_NAME:${product}>"
    )
endforeach()
