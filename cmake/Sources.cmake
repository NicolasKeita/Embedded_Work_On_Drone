# Sources.cmake — source lists of the simulation, control, safety and SIL modules plus
# the SIL/simulation test suites (.cppm + .cpp kept together in the same variable).

# Sources of the simulation module (.cppm + .cpp kept together).
set(SIMULATION_FILES
    Src/Simulation/Aircraft.cppm
    Src/Simulation/Aircraft-Core.cpp
    Src/Simulation/Aircraft-Physics.cpp
    Src/Simulation/PhysicsDispersion.cppm
    Src/Simulation/PhysicsDispersion.cpp
)

# Sources of the flight control module (.cppm + .cpp kept together in the same variable).
set(CONTROL_FILES
    Src/Control/FlightController.cppm
    Src/Control/FlightController-Core.cpp
    Src/Control/FlightController-Loops.cpp
    Src/Control/FlightController-Mission.cpp
    Src/Control/Types/FlightControllerTypes.cppm
    Src/Control/Types/FlightControllerTypes.cpp
)

# Sources of the FC2 safety subsystem (.cppm + .cpp kept together).
set(SAFETY_FILES
    Src/Safety/HealthMonitor.cppm
    Src/Safety/HealthMonitor-Core.cpp
    Src/Safety/HealthMonitor-Evaluate.cpp
    Src/Safety/HealthMonitor-Report.cpp
    Src/Safety/HealthMonitor-Names.cpp
    Src/Safety/SafetyManager.cppm
    Src/Safety/SafetyManager.cpp
)

# Sources of the SIL fault injection and simulation engine (.cppm + .cpp kept
# together in the same variable).
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
    Src/SIL/Core/FunctionalScenarios.cppm
    Src/SIL/Core/FunctionalScenarios.cpp
    Src/SIL/Core/Telemetry/Telemetry.cppm
    Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
    Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
    Src/SIL/Core/Telemetry/SilTelemetry.cppm
    Src/SIL/Core/Telemetry/SilTelemetry.cpp
    Src/SIL/Core/Comms/CommsBus.cppm
    Src/SIL/Core/Comms/CommsBus.cpp
    Src/SIL/Core/Events/SilEvents.cppm
    Src/SIL/Core/Events/SilEvents-Levels.cpp
    Src/SIL/Core/Events/SilEvents-Names.cpp
    Src/SIL/Core/Events/SilEvents-Recorder.cpp
    Src/SIL/Faults/FaultInjectors.cppm
    Src/SIL/Faults/FaultInjectors-Base.cpp
    Src/SIL/Faults/FaultInjectors-Factory.cpp
    Src/SIL/Faults/FaultInjectors-Injectors.cpp
    Src/SIL/Reporting/SilReporting.cppm
    Src/SIL/Reporting/SilReporting-Format.cpp
    Src/SIL/Reporting/SilReporting-Export.cpp
    Src/SIL/Reporting/SilReporting-File.cpp
    Src/SIL/Reporting/Json/SilReporting-Json.cpp
    Src/SIL/Reporting/Json/SilReporting-Json-Records.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace-Text.cpp
    Src/SIL/Reporting/Trace/SilReporting-Trace-Details.cpp
    Src/SIL/Reporting/Report/SilReportingReport.cppm
    Src/SIL/Reporting/Report/SilReportingReport-Sections.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Format.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Markdown.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Csv.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Telemetry.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Events.cpp
    Src/SIL/Reporting/Report/SilReportingReport-Table.cpp
    Src/SIL/Reporting/Report/Events/SilReportingReport-EventRows.cpp
)

# Sources of the FC1 validation campaign tooling (statistical coverage of the fault
# scenarios over the SIL runner).
set(VALIDATION_FILES
    Src/SIL/Validation/FlightControlValidation.cppm
    Src/SIL/Validation/FlightControlValidation-Runner.cpp
    Src/SIL/Validation/FlightControlValidation-Collector.cpp
    Src/SIL/Validation/FlightControlValidation-Generator.cpp
    Src/SIL/Validation/FlightControlValidation-Classify.cpp
    Src/SIL/Validation/FlightControlValidation-CsvRows.cpp
    Src/SIL/Validation/FlightControlValidation-CsvWriter.cpp
    Src/SIL/Validation/Sampling/FlightControlValidation-Sampling.cpp
    Src/SIL/Validation/Sampling/FlightControlValidation-Conditions.cpp
    Src/SIL/Validation/Types/ValidationTypes.cppm
    Src/SIL/Validation/Types/ValidationTypes.cpp
)

# Shared test harness module (checks, logging, run loop) and the non-technical
# scenario brief lookup printed before each run.
set(TEST_HARNESS_FILES
    Tests/TestHarness.cppm
    Tests/TestHarness-Target.cpp
    Tests/TestHarness-Checks.cpp
    Tests/TestHarness-Logging.cpp
    Tests/ScenarioBrief.cppm
    Tests/ScenarioBrief.cpp
    Tests/ScenarioBrief-Nominal.cpp
    Tests/ScenarioBrief-Autonomous.cpp
    Tests/ScenarioBrief-Faults.cpp
)

# Harness, physics scenarios and autonomous scenarios of the validation program
# (.cppm + .cpp kept together in the same variable).
set(TEST_SIMULATION_FILES
    ${TEST_HARNESS_FILES}
    Tests/Scenarios/Scenarios.cppm
    Tests/Scenarios/Scenarios-Cases.cpp
    Tests/Scenarios/Scenarios-Catalog.cpp
    Tests/Mission/MissionRunner.cppm
    Tests/Mission/MissionRunner-Runner.cpp
    Tests/Mission/MissionRunner-Loop.cpp
    Tests/Mission/MissionRunner-Trace.cpp
    Tests/Mission/MissionRunner-Support.cpp
    Tests/Scenarios/FlightScenarios.cppm
    Tests/Scenarios/FlightScenarios-Reference.cpp
    Tests/Scenarios/FlightScenarios-Axes.cpp
    Tests/Scenarios/FlightScenarios-Mission.cpp
)

# Deterministic SIL test suite (NOMINAL-001..010 nominal, FAULT_INJECTOR-001..004 fault
# injection) plus the observability and telemetry suites.
set(TEST_SIL_FILES
    Tests/Sil/SilScenarios.cppm
    Tests/Sil/SilScenarios-Core.cpp
    Tests/Sil/SilScenarios-Safety.cpp
    Tests/Sil/SilScenarios-Suite.cpp
    Tests/Sil/SilScenarios-Run.cpp
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

# Sources of the application (App module + all subsystems).
set(SRC_FILES
    Src/App/Application.cppm
    Src/App/Application.cpp
    ${SIMULATION_FILES}
    ${CONTROL_FILES}
    ${SAFETY_FILES}
    ${SIL_FILES}
)
