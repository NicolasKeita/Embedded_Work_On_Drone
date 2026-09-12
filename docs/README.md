# Documentation Index

Start here. The repository has two layers of documentation:

1. **Canonical (current implementation)** — read first; describes what is
   actually built and runnable.
2. **Reference** — the safety/failure analysis and vocabulary.
3. **Evolution notes** — earlier design notes that describe the *intended
   roadmap* (some of which is not yet in code).

## 1. Canonical — current implementation

| Document | Answers |
| :-- | :-- |
| [../README.md](../README.md) | What is the project, how to build/demo, current status |
| [architecture/overview.md](architecture/overview.md) | What is the software architecture? FC1/FC2 responsibilities, layering, execution model |
| [architecture/architecture.svg](architecture/architecture.svg) | Architecture diagram |
| [validation/scenarios.md](validation/scenarios.md) | Which scenarios can be launched on SIL/HIL, and what each does |
| [validation/test_matrix.md](validation/test_matrix.md) | What exactly has been validated? |
| [validation/sil.md](validation/sil.md) | What does SIL prove? |
| [validation/hil.md](validation/hil.md) | What is the real HIL architecture and timing? |
| [validation/monte_carlo.md](validation/monte_carlo.md) | What does Monte Carlo do? |
| [validation/validation_report.md](validation/validation_report.md) | Executive validation report |
| [demonstrations/fault_recovery.md](demonstrations/fault_recovery.md) | Degradation + recovery demo (sensor fault) |
| [demonstrations/mission_abort.md](demonstrations/mission_abort.md) | Mission abort demo (FC1 failure) |

## 2. Reference

| Document | What |
| :-- | :-- |
| [fmeca/fmeca.md](fmeca/fmeca.md) | **FMECA** — canonical failure-mode analysis (FM-01..FM-07), severity/detectability/occurrence, coverage, limits. Not duplicated elsewhere. |
| [safety/fault_taxonomy.md](safety/fault_taxonomy.md) | Canonical fault/detection/safety vocabulary (failure mode ≠ detection ≠ response). |

## 3. Evolution notes (intended roadmap; not all in code)

> These describe the *planned evolution* (independent OS processes, UDP/CAN
> transport, physical STM32, FreeRTOS tasks). The **current** implementation is
> single-process with in-process simulated communication and a host FC emulator;
> for the current state, always read [architecture/overview.md](architecture/overview.md).

| Document | Scope / caveat |
| :-- | :-- |
| [system/distributed_architecture.md](system/distributed_architecture.md) | FC1/FC2 split rationale; describes a future multi-process/UDP topology **not** in code |
| [system/communication.md](system/communication.md) | ITransport/UDPTransport/CANTransport design; actual transport is `CommsBus`/`LoopbackTransport`/`HilTransport` |
| [system/fault_handling.md](system/fault_handling.md) | Safety state machine (HEALTHY/DEGRADED/SAFE; FAILED removed) — accurate |
| [system/mission.md](system/mission.md) | Mission phases — accurate (some TBD placeholders) |
| [system/aircraft.md](system/aircraft.md) · [system/flight_dynamics.md](system/flight_dynamics.md) · [system/scheduling.md](system/scheduling.md) · [system/software_interfaces.md](system/software_interfaces.md) | Physics/interface/scheduling notes |
| [hil/hil_architecture.md](hil/hil_architecture.md) | Target STM32/FreeRTOS HIL — **future**; current HIL is the host emulator (see [validation/hil.md](validation/hil.md)) |
| [hil/hil_protocol.md](hil/hil_protocol.md) | HIL-Proto wire protocol (Sensor/Actuator packets, CRC16) — **implemented** |
| [hil/hil_validation.md](hil/hil_validation.md) · [hil/hil_runner_design.md](hil/hil_runner_design.md) · [hil/hil_code_guide.md](hil/hil_code_guide.md) | HIL validation/design/code guide |
| [hil/hil_runner_results.md](hil/hil_runner_results.md) | Representative HIL output (clearly labeled; superseded by [validation/hil.md](validation/hil.md)) |
| [test_simulation.md](test_simulation.md) | Original SIL_RUNNER usage reference |

## Toolchain note

Building requires a **C++23-modules** toolchain (`import std;`): MSVC 2026 or
GCC 16+. GCC ≤ 12 cannot compile this project. See [../README.md](../README.md)
for build instructions.
