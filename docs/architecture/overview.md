# Architecture Overview

> **This document is the canonical reference for the *current* implementation.**
> It describes what is actually built and runnable today: a single deterministic
> engine, in-process simulated communication, and a host FC emulator for HIL.
> Earlier design notes under `docs/system/` and `docs/hil/hil_architecture.md`
> describe the *intended evolution* (independent OS processes, UDP transport,
> physical STM32, FreeRTOS). Those are referenced as the roadmap, not as the
> current state — see [Limitations](#6-known-limitations) and
> [`docs/README.md`](../README.md).

## 1. System purpose

The **Distributed Flight Control & Safety Testbed** is a deterministic C++23
software testbed for a simplified *Heliblade-like* single-rotor VTOL aircraft.
Its purpose is **not** to predict real flight, but to validate the *flight
control, health monitoring and safety-management software* of a dual-computer
(FC1/FC2) architecture against a software aircraft model, using deterministic
fault-injection scenarios, statistical (Monte Carlo) dispersion and a
real-time closed-loop (HIL) runner.

The nominal mission is:

```text
TAKEOFF → CLIMB → STATION_KEEPING → COMPLETE
```

On a critical failure the system aborts to a safe state:

```text
FAILURE → DETECTION → DIAGNOSIS → SAFETY ACTION → SAFE_MODE / RECOVERY
```

## 2. High-level architecture

Everything runs in **one process**. A single static library, `flight_sim_core`,
contains the simulation physics, the flight controller, the safety subsystem
and the SIL orchestration engine. Three executables link this one engine so
that SIL, Monte Carlo and HIL share identical control and safety code.

```text
                 ┌──────────────────────────────────────┐
                 │            Aircraft Model            │
                 │  (simplified VTOL physics + truth)  │
                 └──────────────────┬───────────────────┘
                                    │  physics ground truth (AircraftState)
                                    ▼
                              ┌───────────┐
                              │  Sensors  │  SensorTelemetry (only data FC1 sees)
                              └─────┬─────┘
                                    │
                 ┌──────────────────▼───────────────────┐
                 │                 FC1                  │
                 │   FlightController (control + mission)│
                 │   cascaded PID: altitude / X / Y     │
                 │   mission state machine              │
                 └──────────────────┬───────────────────┘
                                    │  ControlCommand (wing RPM + L/R servos)
                                    ▼
                              ┌───────────┐
                              │ Actuators  │  (efficiency scaling, physics step)
                              └─────┬─────┘
                                    │
                                    ▼
                              Aircraft Model

   FC1 ──heartbeat/status──► CommsBus (simulated, seeded packet loss) ──► FC2
   FC2 ──SafetyCommand────► FC1  (thrust margin / mission abort / safe descent)

                 ┌──────────────────────────────────────┐
                 │                 FC2                   │
                 │  HealthMonitor  (detection)           │
                 │  SafetyManager  (diagnosis + action)   │
                 └──────────────────────────────────────┘
```

* **FC1** (`sim::control::FlightController`) consumes only the *sensor* path,
  never the physics ground truth, and emits `ControlCommand` plus a sequenced
  heartbeat.
* **FC2** (`sim::safety::HealthMonitor` + `sim::safety::SafetyManager`) is the
  independent supervision computer: it detects anomalies through the
  communication bus and the sensor telemetry, then commands a safety action
  back to FC1.
* The **communication** between FC1 and FC2 is a **simulated in-process bus**
  (`sim::sil::CommsBus`) with link cutoff and seeded packet loss. There are no
  sockets, no UDP, and no separate OS processes in the current code.

## 3. Responsibilities

| Component | Module | Responsibility |
| :--- | :--- | :--- |
| Aircraft model | `Aircraft`, `PhysicsDispersion` | Simplified VTOL dynamics (thrust from RPM, pitch/roll from servo mean/differential), actuator lag, dispersion hooks for Monte Carlo. Owns the physics ground truth. |
| Sensor layer | `Telemetry` (`make_telemetry`, `validate`, `apply_corruption`) | Produces `SensorTelemetry` from the physics state; range/NaN validation; environment-side corruption. The **only** data path FC1 may consume. |
| Actuator layer | `Aircraft::set_command` / `update` | Applies the commanded RPM + servos (scaled by actuator efficiency and the safety thrust margin), integrates the physics step. |
| FC1 | `FlightController` | Cascaded control (altitude PID → RPM; position PID → tilt → attitude → servo mix) and the mission state machine. |
| FC2 — detection | `HealthMonitor` | Heartbeat/communication supervision, sensor validation, sustained actuator-mismatch detection. Detection is **agnostic**: it never sees the fault injector. |
| FC2 — safety | `SafetyManager` | Maps the health diagnosis to a safety mode/action (`NORMAL` / `COMPENSATED` / `SAFE_MODE`) and emits the `SafetyCommand` (thrust margin, mission abort, safe descent). |
| Communication | `CommsBus` (SIL), `LoopbackTransport` + `HilTransport` (HIL) | Sequenced message delivery with link state, packet loss and transport latency statistics. |
| Fault injector | `FaultInjectors` | Applies the *physical representation* of a failure mode to the simulated environment (FC1 liveness, link state, sensor corruption, actuator efficiency). |
| SIL runner | `SilRunner` | Fixed-time-step closed loop at max CPU speed: Aircraft → injectors → sensors/comms → FC1 → FC2 → actuators. Purely observational logging. |
| HIL runner | `HilRunner` | Real-time closed loop (wall-clock paced) against a **host FC emulator** over a byte-framed loopback transport, with per-step timing statistics. |
| Monte Carlo runner | `MonteCarloCampaign` + `DispersionGenerator` | Repeated dispersed physics-scenario runs with reproducible seeds, collecting control-quality metrics. |

## 4. Software layering

```text
Application / Mission    FlightController mission state machine
Control                 cascaded PID loops (altitude, X, Y, attitude)
Safety                  HealthMonitor (detection) · SafetyManager (action)
Communication           CommsBus (SIL) · HilTransport/LoopbackTransport (HIL)
Interfaces / HAL        SensorInput · ActuatorOutput · Clock · byte Transport
Simulation / Platform   Aircraft physics · PhysicsDispersion · SimClock
```

The HAL interfaces (`SensorInput`, `ActuatorOutput`, `Clock`, byte `Transport`)
are abstract; the current concrete implementations are the in-process `Sim.*`
mocks (`SimSensorInput`, `SimActuatorOutput`, `SimClock`, `LoopbackTransport`).
This abstraction is what lets the same FC1 control code run in SIL, in Monte
Carlo and in the HIL host emulator.

## 5. Execution model

The SIL/HIL loop runs one **fixed time step** (`dt = 0.01 s`, 100 Hz) and
applies, per step, the pipeline below. SIL executes as fast as the CPU allows;
HIL paces each step to an absolute wall-clock deadline.

```text
for each step (dt):
  1. apply_injectors   – reset env to nominal, re-inject active faults (clears temporary faults)
  2. update_fc1        – sample sensors (corruption applied), validate, compute control, publish heartbeat
  3. update_monitoring – HealthMonitor.evaluate → SafetyManager.update → record events
  4. apply_actuators   – SAFE_MODE → controlled descent; COMPENSATED → thrust margin; scale by efficiency; integrate
  5. update_metrics    – error aggregates, mission transitions, dual telemetry sampling (sensor + truth)
```

| Activity | Rate / period | Notes |
| :--- | :--- | :--- |
| Control loop | 100 Hz / 10 ms (`SilConfig::dt`, `HilConfig::dt_s`) | Single-threaded, fixed step. |
| Heartbeat / status | one sequenced message per step while FC1 alive | `CommsBus::publish`. |
| Telemetry sampling | 20 Hz internal (`telemetry_rate_hz`), 1 Hz human-readable report | Dual stream: sensor-observed + physics ground truth. |
| Health evaluation | every step | Detection flags; sensor/actuator flags are continuous, comms/heartbeat flags are **latched**. |
| Safety update | every step | `NORMAL`/`COMPENSATED` reversible; `SAFE_MODE` **irreversible**. |

There is no RTOS and no task scheduler in the current code: the "tasks"
(SensorTask/ControlTask/MissionTask/HealthTask/…) referenced in the legacy
design notes are a single sequential pipeline here. The HIL runner's
`--clock Fast` mode advances instantly for deterministic unit tests while still
exercising the same pipeline and deadline accounting.

## 6. Known limitations

These are genuine limitations of the current implementation, not future plans:

* **Single process, no OS-level isolation.** FC1 and FC2 are in-memory objects,
  not independent processes. The legacy `docs/system/distributed_architecture.md`
  multi-process/UDP design is not realised in code.
* **No physical STM32.** HIL runs against an in-process **host FC emulator**
  over a loopback byte channel. Physical-MCU timing, RTOS scheduling and real
  peripherals are not validated. See [`validation/hil.md`](../validation/hil.md).
* **Simplified dynamics.** The aircraft model is a deliberately simple VTOL
  approximation for control-software development, not a flight-performance model.
* **Latched critical faults.** `FC1_HEARTBEAT_TIMEOUT` and
  `COMMUNICATION_TIMEOUT` flags are latched; once `SAFE_MODE` is engaged the
  mission abort is irreversible. Only sensor (and, in principle, actuator)
  faults are recoverable.
* **Limited fault coverage.** Only the altitude/barometer sensor channel and
  the main-rotor actuator are injectable; IMU/GNSS/RPM/servo faults and the
  documented `CONTROL_DEADLINE_MISSED` / `INVALID_NUMERICAL_STATE` modes have
  no injection path. See the [FMECA](../fmeca/fmeca.md).
* **No local task watchdog.** Liveness is supervised remotely by heartbeat;
  there is no local per-task watchdog.
* **Numerical integrity not guarded.** There is no generic `isfinite` guard on
  position/velocity/attitude/command.

## 7. Design principles

* **Separation of control logic and scheduling.** The `FlightController` knows
  nothing about how/when it is stepped; SIL, HIL and Monte Carlo drive it.
* **Interface-based hardware abstraction.** `SensorInput`/`ActuatorOutput`/
  `Clock`/`Transport` decouple the FC from the concrete channel.
* **Detection ≠ safety response.** `HealthMonitor` only *detects*;
  `SafetyManager` only *reacts*. They are separate components (see
  [`safety/fault_taxonomy.md`](../safety/fault_taxonomy.md)).
* **Deterministic testability.** Seeded RNG, fixed step, no heap in the loop,
  no exceptions (`-fno-exceptions -fno-rtti`); errors are `std::expected`/
  `std::optional`.
* **Reusable control code across SIL/HIL.** The same `flight_sim_core` engine
  and the same safety/health detection chain back all three runners.

For the failure → detection → response chain and the per-mode analysis, see
the [FMECA](../fmeca/fmeca.md) and the
[fault taxonomy](../safety/fault_taxonomy.md). For the diagram, see
[`architecture.svg`](architecture.svg).

