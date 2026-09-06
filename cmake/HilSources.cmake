# HilSources.cmake — source lists of the HIL subsystem (.cppm + .cpp kept together in
# the same variable). The HIL tree is organised by responsibility: Clock, Config,
# Events, Model, Telemetry, Transport, Target, Runner (+ Context/Events/Report), Cli
# and FcHost.

# Hardware abstraction layer (byte transport, sensor/actuator interfaces).
set(HAL_FILES
    Src/Embedded/Hal/HalTypes.cppm
    Src/Embedded/Hal/SensorInput.cppm
    Src/Embedded/Hal/ActuatorOutput.cppm
    Src/Embedded/Hal/Clock.cppm
)

# HIL-Proto wire protocol (transport interface, frame layout, codec, parser).
set(HIL_PROTOCOL_FILES
    Src/Embedded/Transport/Transport.cppm
    Src/Embedded/Transport/HilProtocol.cppm
    Src/Embedded/Transport/Parser/HilProtocolParser.cppm
    Src/Embedded/Transport/Parser/HilProtocolParser-Crc.cpp
    Src/Embedded/Transport/Parser/HilProtocolParser-Sync.cpp
    Src/Embedded/Transport/Parser/HilProtocolParser-Frame.cpp
    Src/Embedded/Transport/Codec/HilProtocolCodec.cppm
    Src/Embedded/Transport/Codec/HilProtocolCodec-Sensor.cpp
    Src/Embedded/Transport/Codec/HilProtocolCodec-Actuator.cpp
)

# In-process simulated hardware (loopback channel, simulated sensor/actuator/clock).
set(HIL_SIM_FILES
    Src/Embedded/Sim/SimSensorInput.cppm
    Src/Embedded/Sim/SimSensorInput.cpp
    Src/Embedded/Sim/SimActuatorOutput.cppm
    Src/Embedded/Sim/SimActuatorOutput.cpp
    Src/Embedded/Sim/SimClock.cppm
    Src/Embedded/Sim/LoopbackTransport.cppm
    Src/Embedded/Sim/LoopbackTransport.cpp
)

# Byte-channel infrastructure shared by the smoke test, the runner and the host FC
# emulator (HAL + wire protocol + simulated hardware; excludes the one-step Demo).
set(HIL_CHANNEL_FILES
    ${HAL_FILES}
    ${HIL_PROTOCOL_FILES}
    ${HIL_SIM_FILES}
)

# One-step lockstep demo (packet codec + transport connectivity smoke test).
set(HIL_DEMO_FILES
    Src/Embedded/Demo/Demo.cppm
    Src/Embedded/Demo/Demo-Sensor.cpp
    Src/Embedded/Demo/Demo-Actuator.cpp
    Src/Embedded/Demo/Demo-Report.cpp
    Src/Embedded/Demo/Demo-Step.cpp
)

set(HIL_FILES
    ${HIL_CHANNEL_FILES}
    ${HIL_DEMO_FILES}
)

# HIL clock and per-step timing statistics.
set(HIL_CLOCK_FILES
    Src/Embedded/Hil/Clock/HilClock.cppm
    Src/Embedded/Hil/Clock/HilClock.cpp
    Src/Embedded/Hil/Clock/HilTiming.cppm
    Src/Embedded/Hil/Clock/HilTiming.cpp
)

# HIL configuration and scenario registry.
set(HIL_CONFIG_FILES
    Src/Embedded/Hil/Config/HilConfig.cppm
    Src/Embedded/Hil/Config/HilConfig.cpp
    Src/Embedded/Hil/Config/HilScenarios.cppm
    Src/Embedded/Hil/Config/HilScenarios.cpp
)

# Structured HIL events and trace recorder.
set(HIL_EVENTS_FILES
    Src/Embedded/Hil/Events/HilEvents.cppm
    Src/Embedded/Hil/Events/HilEvents.cpp
)

# Simulated sensor chain of the HIL runner.
set(HIL_MODEL_FILES
    Src/Embedded/Hil/Model/HilSensorModel.cppm
    Src/Embedded/Hil/Model/HilSensorModel.cpp
)

# Structured HIL telemetry streams and dual sampler.
set(HIL_TELEMETRY_FILES
    Src/Embedded/Hil/Telemetry/HilTelemetry.cppm
    Src/Embedded/Hil/Telemetry/HilTelemetry.cpp
)

# Frame-level HIL transport (send/receive + communication statistics).
set(HIL_TRANSPORT_MODULE_FILES
    Src/Embedded/Hil/Transport/HilTransport.cppm
    Src/Embedded/Hil/Transport/HilTransport-Stats.cpp
    Src/Embedded/Hil/Transport/HilTransport-Receive.cpp
    Src/Embedded/Hil/Transport/HilTransport-Frames.cpp
)

# FC target abstraction and the host FC emulator target.
set(HIL_TARGET_FILES
    Src/Embedded/Hil/Target/HilFcTarget.cppm
    Src/Embedded/Hil/Target/HilFcTarget-Host.cpp
    Src/Embedded/Hil/Target/HilFcTarget-Respond.cpp
)

# HIL run context and the result/output types.
set(HIL_CONTEXT_FILES
    Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cppm
    Src/Embedded/Hil/Runner/Context/HilRunnerTypes.cpp
    Src/Embedded/Hil/Runner/Context/HilRunnerContext.cppm
    Src/Embedded/Hil/Runner/Context/HilRunnerContext.cpp
)

# Real-time HIL runner (closed loop, step, metrics, finalization).
set(HIL_RUNNER_FILES
    Src/Embedded/Hil/Runner/HilRunner.cppm
    Src/Embedded/Hil/Runner/HilRunner-Context.cpp
    Src/Embedded/Hil/Runner/HilRunner-Finalize.cpp
    Src/Embedded/Hil/Runner/HilRunner-Metrics.cpp
    Src/Embedded/Hil/Runner/HilRunner-Loop.cpp
    Src/Embedded/Hil/Runner/HilRunner-Inject.cpp
    Src/Embedded/Hil/Runner/HilRunner-Step.cpp
    Src/Embedded/Hil/Runner/HilRunner-Apply.cpp
)

# Structured event recording of the HIL runner.
set(HIL_RUNNER_EVENTS_FILES
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents.cppm
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Lifecycle.cpp
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Faults.cpp
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Steps.cpp
)

# Human-readable HIL reporting (header, report, summary, live streaming).
set(HIL_REPORT_FILES
    Src/Embedded/Hil/Runner/Report/HilReport.cppm
    Src/Embedded/Hil/Runner/Report/HilReport-Table.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Header.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Report.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Summary.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Live.cpp
)

# Command-line interface of the hil_runner executable.
set(HIL_CLI_FILES
    Src/Embedded/Hil/Cli/HilRunnerCli.cppm
    Src/Embedded/Hil/Cli/HilRunnerCli-Parse.cpp
    Src/Embedded/Hil/Cli/HilRunnerCli-Config.cpp
)

# Host FC emulator application (stdio frame loop of fc1_hil_host).
set(HIL_FC_HOST_FILES
    Src/Embedded/Hil/FcHost/FcHostApp.cppm
    Src/Embedded/Hil/FcHost/FcHostApp-Frame.cpp
    Src/Embedded/Hil/FcHost/FcHostApp-Loop.cpp
)

# HIL runner infrastructure (config, clock, events, telemetry, timing, sensor model,
# transport adaptors, FC target, run context, orchestrator and scenario registry).
set(HIL_INFRA_FILES
    ${HIL_CLOCK_FILES}
    ${HIL_CONFIG_FILES}
    ${HIL_EVENTS_FILES}
    ${HIL_MODEL_FILES}
    ${HIL_TELEMETRY_FILES}
    ${HIL_TRANSPORT_MODULE_FILES}
    ${HIL_TARGET_FILES}
    ${HIL_CONTEXT_FILES}
    ${HIL_RUNNER_FILES}
    ${HIL_RUNNER_EVENTS_FILES}
    ${HIL_REPORT_FILES}
)

# SIL core reused by the HIL runner (fault vocabulary, bus, sensor chain).
set(SIL_CORE_FILES
    Src/SIL/Core/SilTypes.cppm
    Src/SIL/Core/SilTypes.cpp
    Src/SIL/Core/SilFaultScenario.cppm
    Src/SIL/Core/SilFaultScenario-Core.cpp
    Src/SIL/Core/SilFaultScenario-Names.cpp
    Src/SIL/Core/SilFaultScenario-Semantics.cpp
    Src/SIL/Core/FunctionalScenarios.cppm
    Src/SIL/Core/FunctionalScenarios.cpp
    Src/SIL/Faults/FaultInjectors.cppm
    Src/SIL/Faults/FaultInjectors-Base.cpp
    Src/SIL/Faults/FaultInjectors-Injectors.cpp
    Src/SIL/Faults/FaultInjectors-Factory.cpp
    Src/SIL/Core/Comms/CommsBus.cppm
    Src/SIL/Core/Comms/CommsBus.cpp
    Src/SIL/Core/Telemetry/Telemetry.cppm
    Src/SIL/Core/Telemetry/Telemetry-Chain.cpp
    Src/SIL/Core/Telemetry/Telemetry-Validation.cpp
)

# HIL deterministic test suite (runner, protocol, data-integrity, fault tests).
set(TEST_HIL_FILES
    Tests/Hil/HilTests.cppm
    Tests/Hil/HilTests-Core.cpp
    Tests/Hil/HilTests-Runner.cpp
    Tests/Hil/HilTests-Timing.cpp
    Tests/Hil/HilTests-Protocol.cpp
    Tests/Hil/HilTests-Data.cpp
    Tests/Hil/HilTests-Faults.cpp
)
