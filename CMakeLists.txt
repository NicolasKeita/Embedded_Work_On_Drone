cmake_minimum_required(VERSION 4.1)

project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

# Sources du module de simulation (.cppm + .cpp conserves ensemble).
set(SIMULATION_FILES
    Src/Simulation/Aircraft.cppm
    Src/Simulation/Aircraft.cpp
)

# Sources du module de controle de vol (.cppm + .cpp conserves ensemble dans la meme variable).
set(CONTROL_FILES
    Src/Control/FlightController.cppm
    Src/Control/FlightController-Core.cpp
    Src/Control/FlightController-Loops.cpp
    Src/Control/FlightController-Mission.cpp
)

# Sources du sous-systeme de surete FC2 (.cppm + .cpp conserves ensemble).
set(SAFETY_FILES
    Src/Safety/HealthMonitor.cppm
    Src/Safety/HealthMonitor-Core.cpp
    Src/Safety/HealthMonitor-Evaluate.cpp
    Src/Safety/HealthMonitor-Report.cpp
    Src/Safety/HealthMonitor-Names.cpp
    Src/Safety/SafetyManager.cppm
    Src/Safety/SafetyManager.cpp
)

# Sources du moteur de fault injection et de simulation SIL (.cppm + .cpp
# conserves ensemble dans la meme variable).
set(SIL_FILES
    Src/SIL/Runner/SilRunner.cppm
    Src/SIL/Runner/SilRunner-Context.cpp
    Src/SIL/Runner/Events/SilRunner-Events.cpp
    Src/SIL/Runner/Events/SilRunner-FaultEvents.cpp
    Src/SIL/Runner/Events/SilRunner-Faults.cpp
    Src/SIL/Runner/SilRunner-Fc1.cpp
    Src/SIL/Runner/Events/SilRunner-Heartbeats.cpp
    Src/SIL/Runner/SilRunner-Loop.cpp
    Src/SIL/Runner/SilRunner-Run.cpp
    Src/SIL/Runner/SilRunner-Finalize.cpp
    Src/SIL/Runner/Events/SilRunner-Transitions.cpp
    Src/SIL/Core/SilTypes.cppm
    Src/SIL/Core/SilTypes.cpp
    Src/SIL/Core/Telemetry/Telemetry.cppm
    Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
    Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
    Src/SIL/Core/Telemetry/SilTelemetry.cppm
    Src/SIL/Core/Telemetry/SilTelemetry.cpp
    Src/SIL/Core/CommsBus.cppm
    Src/SIL/Core/CommsBus.cpp
    Src/SIL/Core/Events/SilEvents.cppm
    Src/SIL/Core/Events/SilEvents-Names.cpp
    Src/SIL/Core/Events/SilEvents-Levels.cpp
    Src/SIL/Core/Events/SilEvents-Recorder.cpp
    Src/SIL/Faults/FaultInjectors.cppm
    Src/SIL/Faults/FaultInjectors-Base.cpp
    Src/SIL/Faults/FaultInjectors-Injectors.cpp
    Src/SIL/Faults/FaultInjectors-Factory.cpp
    Src/SIL/Reporting/SilReporting.cppm
    Src/SIL/Reporting/SilReporting-Export.cpp
    Src/SIL/Reporting/SilReporting-File.cpp
    Src/SIL/Reporting/SilReporting-Format.cpp
    Src/SIL/Reporting/Report/SilReporting-Report.cpp
    Src/SIL/Reporting/Report/SilReporting-Report-Sections.cpp
    Src/SIL/Reporting/Report/SilReporting-Report-Telemetry.cpp
    Src/SIL/Reporting/Report/SilReporting-Report-Events.cpp
    Src/SIL/Reporting/Report/SilReporting-Streams.cpp
    Src/SIL/Reporting/Json/SilReporting-Json.cpp
    Src/SIL/Reporting/Json/SilReporting-Json-Records.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace-Text.cpp
)

set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
)

add_executable(HelloWorld Src/main.cpp)
target_sources(HelloWorld
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SRC_FILES}
)

# Harness, scenarios physiques et scenarios autonomes du programme de validation
# (.cppm + .cpp conserves ensemble dans la meme variable).
set(TEST_SIMULATION_FILES
    Tests/TestHarness.cppm
    Tests/TestHarness.cpp
    Tests/Scenarios/Scenarios.cppm
    Tests/Scenarios/Scenarios-Cases.cpp
    Tests/Scenarios/Scenarios-Catalog.cpp
    Tests/Mission/MissionRunner.cppm
    Tests/Mission/MissionRunner-Runner.cpp
    Tests/Mission/MissionRunner-Loop.cpp
    Tests/Mission/MissionRunner-Support.cpp
    Tests/Scenarios/FlightScenarios.cppm
    Tests/Scenarios/FlightScenarios-Axes.cpp
    Tests/Scenarios/FlightScenarios-Mission.cpp
)

# Exercice de validation des scenarios A a J (etapes 5 et 6).
add_executable(test_simulation Tests/test_simulation.cpp)
target_sources(test_simulation
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SIMULATION_FILES}
            ${CONTROL_FILES}
            ${TEST_SIMULATION_FILES}
)

# Suite de tests SIL deterministes SIL-001 a SIL-005 (etape 11) et scenarios
# d'observabilite.
set(TEST_SIL_FILES
    Tests/Sil/SilScenarios.cppm
    Tests/Sil/SilScenarios-Core.cpp
    Tests/Sil/SilScenarios-Safety.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Support.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Heartbeat.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Dropped.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Faults.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Comms.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Fields.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Neutrality.cpp
    Tests/Sil/Observability/SilScenarios-Observability-Trace.cpp
    Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Sampling.cpp
    Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-FaultPath.cpp
    Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Hold.cpp
    Tests/Sil/Observability/Telemetry/SilScenarios-Observability-Telemetry-Support.cpp
)

add_executable(test_sil Tests/test_sil.cpp)
target_sources(test_sil
    PRIVATE
        FILE_SET CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${SIMULATION_FILES}
            ${CONTROL_FILES}
            ${SAFETY_FILES}
            ${SIL_FILES}
            Tests/TestHarness.cppm
            Tests/TestHarness.cpp
            ${TEST_SIL_FILES}
)

foreach(target HelloWorld test_simulation test_sil)
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

install(TARGETS HelloWorld RUNTIME DESTINATION bin)