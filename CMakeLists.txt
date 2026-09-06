cmake_minimum_required(VERSION 4.1)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
    "f35a9ac6-8463-4d38-8eec-5d6008153e7d"
    CACHE STRING "Enable experimental C++ import std support")
project(HelloWorld VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


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
    Src/Control/Types/FlightControllerTypes.cppm
    Src/Control/Types/FlightControllerTypes.cpp
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
    Src/SIL/Runner/SilRunner-Fc1.cpp
    Src/SIL/Runner/SilRunner-Loop.cpp
    Src/SIL/Runner/SilRunner-Run.cpp
    Src/SIL/Runner/SilRunner-Finalize.cpp
    Src/SIL/Runner/Context/SilRunnerContext.cppm
    Src/SIL/Runner/Context/SilRunnerContext.cpp
    Src/SIL/Runner/Events/SilRunnerEvents.cppm
    Src/SIL/Runner/Events/SilRunnerEvents-Lifecycle.cpp
    Src/SIL/Runner/Events/Faults/SilRunnerEvents-Faults.cpp
    Src/SIL/Runner/Events/Faults/SilRunnerEvents-FaultInfo.cpp
    Src/SIL/Runner/Events/Faults/SilRunnerEvents-FaultReasons.cpp
    Src/SIL/Runner/Events/SilRunnerEvents-Watchdog.cpp
    Src/SIL/Runner/Events/SilRunnerEvents-Heartbeats.cpp
    Src/SIL/Runner/Events/SilRunnerEvents-Transitions.cpp
    Src/SIL/Runner/Events/SilRunnerEvents-Health.cpp
    Src/SIL/Core/SilTypes.cppm
    Src/SIL/Core/SilTypes.cpp
    Src/SIL/Core/SilFaultScenario.cppm
    Src/SIL/Core/SilFaultScenario-Core.cpp
    Src/SIL/Core/SilFaultScenario-Names.cpp
    Src/SIL/Core/SilFaultScenario-Semantics.cpp
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
    Src/SIL/Reporting/Json/SilReporting-Json.cpp
    Src/SIL/Reporting/Json/SilReporting-Json-Records.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace-Details.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace-Text.cpp
    Src/SIL/Reporting/Report/SilReportingReport.cppm
    Src/SIL/Reporting/Report/SilReportingReport-Format.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Markdown.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Sections.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Events.cpp
    Src/SIL/Reporting/Report/Events/SilReportingReport-EventRows.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Telemetry.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Table.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Csv.cpp
)

set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
)

# Sources du sous-systeme de validation statistique Monte-Carlo SIL (.cppm +
# .cpp conserves ensemble dans la meme variable ; le vocabulaire commun vit
# dans le sous-module ValidationTypes, l'interface principale re-exporte le
# tout et regroupe le generateur, le collecteur et le runner).
set(VALIDATION_FILES
    Src/SIL/Validation/FlightControlValidation.cppm
    Src/SIL/Validation/Types/ValidationTypes.cppm
    Src/SIL/Validation/Types/ValidationTypes.cpp
    Src/SIL/Validation/FlightControlValidation-Generator.cpp
    Src/SIL/Validation/Sampling/FlightControlValidation-Sampling.cpp
    Src/SIL/Validation/Sampling/FlightControlValidation-Conditions.cpp
    Src/SIL/Validation/FlightControlValidation-Collector.cpp
    Src/SIL/Validation/FlightControlValidation-Runner.cpp
    Src/SIL/Validation/FlightControlValidation-Classify.cpp
    Src/SIL/Validation/FlightControlValidation-CsvRows.cpp
    Src/SIL/Validation/FlightControlValidation-CsvWriter.cpp
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
target_add_mixed_sources(test_simulation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${TEST_SIMULATION_FILES}
)
set_property(TARGET test_simulation PROPERTY CXX_SCAN_FOR_MODULES ON)

# Suite de tests SIL deterministes SIL-001 a SIL-005 (etape 11) et scenarios
# d'observabilite.
set(TEST_SIL_FILES
    Tests/Sil/SilScenarios.cppm
    Tests/Sil/SilScenarios-Core.cpp
    Tests/Sil/SilScenarios-Safety.cpp
    Tests/Sil/Observability/SilObservability.cppm
    Tests/Sil/Observability/SilObservability-Support.cpp
    Tests/Sil/Observability/SilObservability-Heartbeat.cpp
    Tests/Sil/Observability/SilObservability-Dropped.cpp
    Tests/Sil/Observability/Faults/SilObservability-Faults.cpp
    Tests/Sil/Observability/Faults/SilObservability-FaultMetadata.cpp
    Tests/Sil/Observability/SilObservability-Comms.cpp
    Tests/Sil/Observability/SilObservability-Logging.cpp
    Tests/Sil/Observability/SilObservability-Trace.cpp
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry.cppm
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Sampling.cpp
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-FaultPath.cpp
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Hold.cpp
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Support.cpp
    Tests/Sil/Observability/Telemetry/SilObservabilityTelemetry-Runner.cpp
)

add_executable(test_sil Tests/test_sil.cpp)
target_add_mixed_sources(test_sil PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
    Tests/TestHarness.cppm
    Tests/TestHarness.cpp
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

# Campagne de validation statistique Monte-Carlo sur le moteur SIL : le point
# d'entree Src/SIL/Validation/main.cpp lance N runs deterministes et exporte
# les resultats au format CSV.
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
# HIL Hardware-in-the-Loop skeleton (etape 13).
# Couche d'abstraction materielle (HAL), protocole binaire HIL-Proto v1.0
# (types, parseur, codec), mocks de simulation hote (PC) et demo lockstep.
# Chaque module .cppm garde l'interface ; les implementations vivent dans les
# .cpp apparies (<module>.cpp ou <module>-<Suffix>.cpp).
# ----------------------------------------------------------------------------
set(HIL_FILES
    Src/Embedded/Hal/HalTypes.cppm
    Src/Embedded/Hal/SensorInput.cppm
    Src/Embedded/Hal/ActuatorOutput.cppm
    Src/Embedded/Hal/Clock.cppm
    Src/Embedded/Transport/Transport.cppm
    Src/Embedded/Transport/HilProtocol.cppm
    Src/Embedded/Transport/Parser/HilProtocolParser.cppm
    Src/Embedded/Transport/Parser/HilProtocolParser-Crc.cpp
    Src/Embedded/Transport/Parser/HilProtocolParser-Sync.cpp
    Src/Embedded/Transport/Parser/HilProtocolParser-Frame.cpp
    Src/Embedded/Transport/Codec/HilProtocolCodec.cppm
    Src/Embedded/Transport/Codec/HilProtocolCodec-Sensor.cpp
    Src/Embedded/Transport/Codec/HilProtocolCodec-Actuator.cpp
    Src/Embedded/Sim/SimSensorInput.cppm
    Src/Embedded/Sim/SimSensorInput.cpp
    Src/Embedded/Sim/SimActuatorOutput.cppm
    Src/Embedded/Sim/SimActuatorOutput.cpp
    Src/Embedded/Sim/SimClock.cppm
    Src/Embedded/Sim/LoopbackTransport.cppm
    Src/Embedded/Sim/LoopbackTransport.cpp
    Src/Embedded/Demo/Demo.cppm
    Src/Embedded/Demo/Demo-Sensor.cpp
    Src/Embedded/Demo/Demo-Actuator.cpp
    Src/Embedded/Demo/Demo-Report.cpp
    Src/Embedded/Demo/Demo-Step.cpp
)

# Demonstration : bouclage d'un pas de simulation lockstep
# (SensorPacket -> parser -> ISensorInput -> FC factice -> IActuatorOutput ->
#  ActuatorPacket).
add_executable(hil_demo Src/Embedded/Main.cpp)
target_add_mixed_sources(hil_demo PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_FILES}
)
set_property(TARGET hil_demo PROPERTY CXX_SCAN_FOR_MODULES ON)
set_property(TARGET hil_demo PROPERTY CXX_MODULE_STD ON)

if(MSVC)
    target_compile_options(hil_demo PRIVATE
        /W4
        /external:W0
        /external:anglebrackets
    )
else()
    target_compile_options(hil_demo PRIVATE -Wall -Wextra -Wpedantic)
endif()

target_compile_features(hil_demo PRIVATE cxx_std_23)
