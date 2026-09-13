# HilSources.cmake — source lists of the HIL subsystem (.cppm + .cpp kept together in
# the same variable). The HIL tree is organised by responsibility: Clock, Config,
# Events, Model, Telemetry, Transport, Target, Runner (+ Context/Events/Report) and Cli.

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
    Src/Embedded/InterFc/InterFcLink.cppm
    Src/Embedded/InterFc/InterFcLink-Codec.cpp
    Src/Embedded/InterFc/InterFcLink-Crc.cpp
    Src/Embedded/InterFc/InterFcLink-Parser.cpp
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

# Byte-channel infrastructure (HAL + wire protocol + simulated hardware).
set(HIL_CHANNEL_FILES
    ${HAL_FILES}
    ${HIL_PROTOCOL_FILES}
    ${HIL_SIM_FILES}
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
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher.cppm
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Sockets.cpp
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Crypto.cpp
    Src/Embedded/Hil/Telemetry/TwinWebSocketPublisher-Handshake.cpp
)

# Frame-level HIL transport (send/receive + communication statistics).
set(HIL_TRANSPORT_MODULE_FILES
    Src/Embedded/Hil/Transport/Serial/Stm32Discovery.cppm
    Src/Embedded/Hil/Transport/Serial/Stm32Discovery.cpp
    Src/Embedded/Hil/Transport/Serial/SerialTransport.cppm
    Src/Embedded/Hil/Transport/Serial/SerialTransport-Port.cpp
    Src/Embedded/Hil/Transport/Serial/SerialTransport-Channel.cpp
    Src/Embedded/Hil/Transport/HilTransport.cppm
    Src/Embedded/Hil/Transport/HilTransport-Stats.cpp
    Src/Embedded/Hil/Transport/HilTransport-Receive.cpp
    Src/Embedded/Hil/Transport/HilTransport-Accept.cpp
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
    Src/Embedded/Hil/Runner/Wiring/HilRunner-Channel.cpp
    Src/Embedded/Hil/Runner/HilRunner-Finalize.cpp
    Src/Embedded/Hil/Runner/HilRunner-Metrics.cpp
    Src/Embedded/Hil/Runner/HilRunner-Loop.cpp
    Src/Embedded/Hil/Runner/HilRunner-Inject.cpp
    Src/Embedded/Hil/Runner/HilRunner-Step.cpp
)

# Safety submodule of the HIL runner (embedded-FC2 supervision selection, health
# dispatch, safety-mode application to the aircraft).
set(HIL_RUNNER_SAFETY_FILES
    Src/Embedded/Hil/Runner/Safety/HilRunnerSafety.cppm
    Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Health.cpp
    Src/Embedded/Hil/Runner/Safety/HilRunnerSafety-Apply.cpp
)

# Structured event recording of the HIL runner.
set(HIL_RUNNER_EVENTS_FILES
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents.cppm
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Lifecycle.cpp
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Faults.cpp
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Detection.cpp
    Src/Embedded/Hil/Runner/Events/HilRunnerEvents-Steps.cpp
)

# Human-readable HIL reporting (header, report, summary, live streaming).
set(HIL_REPORT_FILES
    Src/Embedded/Hil/Runner/Report/HilReport.cppm
    Src/Embedded/Hil/Runner/Report/HilReport-Table.cpp
    Src/Embedded/Hil/Runner/Report/Header/HilReport-Header.cpp
    Src/Embedded/Hil/Runner/Report/Header/HilReport-Config.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Report.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Summary.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Live.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Twin.cpp
    Src/Embedded/Hil/Runner/Report/HilReport-Components.cpp
)

# Command-line interface of the HIL_RUNNER executable.
set(HIL_CLI_FILES
    Src/Embedded/Hil/Cli/HilRunnerCli.cppm
    Src/Embedded/Hil/Cli/HilRunnerCli-Parse.cpp
    Src/Embedded/Hil/Cli/HilRunnerCli-Dispatch.cpp
    Src/Embedded/Hil/Cli/HilRunnerCli-Usage.cpp
    Src/Embedded/Hil/Cli/HilRunnerCli-Config.cpp
    Src/Embedded/Hil/Cli/HilRunnerCli-Interface.cpp
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
    ${HIL_RUNNER_SAFETY_FILES}
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
    Tests/Hil/Transport/HilTests-Transport.cpp
    Tests/Hil/HilTests-Data.cpp
    Tests/Hil/HilTests-Faults.cpp
)
