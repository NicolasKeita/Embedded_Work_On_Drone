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

# HIL transport/HAL/sim infrastructure shared by the smoke test, the runner and the host
# FC emulator (excludes the one-step Demo, which lives in its own smoke test).
set(HIL_TRANSPORT_FILES
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
)

# SIL core reused by the HIL runner (fault vocabulary, bus, sensor chain).
set(SIL_CORE_FILES
    Src/SIL/Core/SilTypes.cppm
    Src/SIL/Core/SilTypes.cpp
    Src/SIL/Core/SilFaultScenario.cppm
    Src/SIL/Core/SilFaultScenario-Core.cpp
    Src/SIL/Core/SilFaultScenario-Names.cpp
    Src/SIL/Core/SilFaultScenario-Semantics.cpp
    Src/SIL/Faults/FaultInjectors.cppm
    Src/SIL/Faults/FaultInjectors-Base.cpp
    Src/SIL/Faults/FaultInjectors-Injectors.cpp
    Src/SIL/Faults/FaultInjectors-Factory.cpp
    Src/SIL/Core/CommsBus.cppm
    Src/SIL/Core/CommsBus.cpp
    Src/SIL/Core/Telemetry/Telemetry.cppm
    Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
    Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
)

# HIL runner infrastructure (config, clock, events, telemetry, timing, sensor model,
# transport adaptors, FC target, orchestrator and scenario registry).
set(HIL_INFRA_FILES
    Src/Embedded/Hil/HilConfig.cppm
    Src/Embedded/Hil/HilConfig.cpp
    Src/Embedded/Hil/HilClock.cppm
    Src/Embedded/Hil/HilClock.cpp
    Src/Embedded/Hil/HilEvents.cppm
    Src/Embedded/Hil/HilEvents.cpp
    Src/Embedded/Hil/HilTelemetry.cppm
    Src/Embedded/Hil/HilTelemetry.cpp
    Src/Embedded/Hil/HilTiming.cppm
    Src/Embedded/Hil/HilTiming.cpp
    Src/Embedded/Hil/HilSensorModel.cppm
    Src/Embedded/Hil/HilSensorModel.cpp
    Src/Embedded/Hil/HilTransport.cppm
    Src/Embedded/Hil/HilTransport.cpp
    Src/Embedded/Hil/HilFcTarget.cppm
    Src/Embedded/Hil/HilFcTarget-Host.cpp
    Src/Embedded/Hil/HilRunnerContext.cppm
    Src/Embedded/Hil/HilRunnerContext.cpp
    Src/Embedded/Hil/HilRunner.cppm
    Src/Embedded/Hil/HilRunner-Context.cpp
    Src/Embedded/Hil/HilRunner-Events.cpp
    Src/Embedded/Hil/HilRunner-Step.cpp
    Src/Embedded/Hil/HilRunner-Loop.cpp
    Src/Embedded/Hil/HilRunner-Metrics.cpp
    Src/Embedded/Hil/HilRunner-Finalize.cpp
    Src/Embedded/Hil/HilRunner-Report.cpp
    Src/Embedded/Hil/HilScenarios.cppm
    Src/Embedded/Hil/HilScenarios.cpp
)

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
add_executable(hil_step_smoke_test Src/Embedded/Main.cpp)
target_add_mixed_sources(hil_step_smoke_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_FILES}
)
hil_target_setup(hil_step_smoke_test)

# HIL runner : real-time closed-loop mission against the FC target.
add_executable(hil_runner Src/Embedded/Hil/HilRunnerMain.cpp)
target_add_mixed_sources(hil_runner PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    ${HIL_INFRA_FILES}
    ${HIL_TRANSPORT_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(hil_runner)

# Host FC emulator target : the real FC core driven over stdio frames. NOT the physical STM32.
add_executable(fc1_hil_host Src/Embedded/Hil/FcHostMain.cpp)
target_add_mixed_sources(fc1_hil_host PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    Src/Embedded/Hil/HilClock.cppm
    Src/Embedded/Hil/HilClock.cpp
    Src/Embedded/Hil/HilConfig.cppm
    Src/Embedded/Hil/HilConfig.cpp
    Src/Embedded/Hil/HilSensorModel.cppm
    Src/Embedded/Hil/HilSensorModel.cpp
    Src/Embedded/Hil/HilScenarios.cppm
    Src/Embedded/Hil/HilScenarios.cpp
    ${HIL_TRANSPORT_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(fc1_hil_host)

# HIL deterministic test suite (runner, protocol, data-integrity, fault tests).
set(TEST_HIL_FILES
    Tests/Hil/HilTests.cppm
    Tests/Hil/HilTests-Core.cpp
    Tests/Hil/HilTests-Runner.cpp
    Tests/Hil/HilTests-Protocol.cpp
    Tests/Hil/HilTests-Data.cpp
    Tests/Hil/HilTests-Faults.cpp
)

add_executable(test_hil Tests/test_hil.cpp)
target_add_mixed_sources(test_hil PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    Tests/TestHarness.cppm
    Tests/TestHarness.cpp
    ${TEST_HIL_FILES}
    ${HIL_INFRA_FILES}
    ${HIL_TRANSPORT_FILES}
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_CORE_FILES}
)
hil_target_setup(test_hil)
