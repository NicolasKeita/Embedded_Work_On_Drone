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
    Src/Control/FlightController.cpp
    Src/Control/FlightControllerLoops.cpp
    Src/Control/FlightControllerMission.cpp
)

# Sources du sous-systeme de surete FC2 (.cppm + .cpp conserves ensemble).
set(SAFETY_FILES
    Src/Safety/HealthMonitor.cppm
    Src/Safety/HealthMonitor.cpp
    Src/Safety/HealthMonitorNames.cpp
    Src/Safety/SafetyManager.cppm
    Src/Safety/SafetyManager.cpp
)

# Sources du moteur de fault injection et de simulation SIL (.cppm + .cpp
# conserves ensemble dans la meme variable).
set(SIL_FILES
    Src/SIL/SilTypes.cppm
    Src/SIL/SilTypes.cpp
    Src/SIL/Telemetry.cppm
    Src/SIL/Telemetry.cpp
    Src/SIL/CommsBus.cppm
    Src/SIL/CommsBus.cpp
    Src/SIL/IFaultInjector.cppm
    Src/SIL/IFaultInjector.cpp
    Src/SIL/FCFailureInjector.cppm
    Src/SIL/FCFailureInjector.cpp
    Src/SIL/CommunicationFaultInjector.cppm
    Src/SIL/CommunicationFaultInjector.cpp
    Src/SIL/SensorFaultInjector.cppm
    Src/SIL/SensorFaultInjector.cpp
    Src/SIL/ActuatorFaultInjector.cppm
    Src/SIL/ActuatorFaultInjector.cpp
    Src/SIL/FaultInjectorFactory.cppm
    Src/SIL/FaultInjectorFactory.cpp
    Src/SIL/SilRunner.cppm
    Src/SIL/SilRunner.cpp
    Src/SIL/SilRunnerLoop.cpp
    Src/SIL/SilReportFormat.cppm
    Src/SIL/SilReportFormat.cpp
    Src/SIL/SilReporting.cppm
    Src/SIL/SilReporting.cpp
    Src/SIL/SilReportingExport.cpp
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
    Tests/Scenarios.cppm
    Tests/Scenarios.cpp
    Tests/ScenarioCatalog.cppm
    Tests/ScenarioCatalog.cpp
    Tests/MissionSupport.cppm
    Tests/MissionSupport.cpp
    Tests/MissionRunner.cppm
    Tests/MissionRunner.cpp
    Tests/FlightScenarios.cppm
    Tests/FlightScenarios.cpp
    Tests/FlightMission.cppm
    Tests/FlightMission.cpp
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

# Suite de tests SIL deterministes SIL-001 a SIL-005 (etape 11).
set(TEST_SIL_FILES
    Tests/SilScenarios.cppm
    Tests/SilScenarios.cpp
    Tests/SilScenariosSafety.cpp
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