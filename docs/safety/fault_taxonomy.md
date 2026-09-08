# Fault Taxonomy and Safety Architecture Vocabulary

This document is the canonical reference for the fault, detection and safety
vocabulary of the testbed. It prepares the codebase for a future FMECA by
keeping three concepts strictly separated in the code, the logs and the
documentation:

```text
Failure Mode ──► Detection Mechanism ──► Diagnosis / Classification
                                           │
                                           ▼
              Recovery / Safe Mode / Degraded Mode ◄── Safety Response
```

- A **Failure Mode** is the root cause being injected or postulated
  (`sim::sil::FailureMode`, module `SilFaultScenario`).
- A **Detection Event** is the output of a detection mechanism
  (`sim::safety::DetectionEvent`, module `HealthMonitor`).
- A **System State** is a health or safety state (`sim::safety::HealthState`,
  `sim::safety::SafetyMode`).
- A **Safety Action** is the reaction commanded after the diagnosis
  (`sim::safety::SafetyAction`, module `SafetyManager`).

These concepts are never mixed: a detection event name never names a root
cause, and a failure mode name never names a detection mechanism.

---

## 1. Fault Domains

Every problem handled by the testbed belongs to exactly one domain
(`sim::sil::FaultDomain`):

| Domain | Meaning | Code representation |
| :--- | :--- | :--- |
| `SYSTEM` | Real aircraft / embedded system failures. **The only domain the FMECA focuses on.** | `sim::sil::FailureMode` scenarios injected through `sim::sil::FaultInjector` |
| `HIL` | Hardware-in-the-loop bench and protocol failures. | `sim::hil::ReceiveResult`, `sim::hil::FrameAcceptanceError`, `sim::hil::HilEventType::HilStepError` |
| `INFRASTRUCTURE` | Test framework, filesystem, CLI or configuration errors. | `sim::sil::SilError`, `sim::sil::InjectorError`, `sim::sil::ReportError`, `sim::hil::HilError`, `sim::sil::validation::FailureReason::RunnerError` |

`sim::sil::failure_mode_domain()` documents that every injectable failure mode
is a `SYSTEM` fault: HIL bench problems and infrastructure errors are reported
through their own typed error channels and are never presented as aircraft
failures.

---

## 2. Failure Modes (SYSTEM domain)

`sim::sil::FailureMode` — the root-cause vocabulary. The injector applies the
*physical representation* of the mode in the simulated environment; detection
stays agnostic (the `HealthMonitor` never sees the injector).

| ID | Failure Mode | Meaning | Injection representation | Parameters |
| :--- | :--- | :--- | :--- | :--- |
| FM-01 | `FC1_UNAVAILABLE` | Primary flight controller no longer operational (crash, stopped execution, no heartbeat). | `fc1_alive = false`: heartbeat and command emission stop. | none |
| FM-02 | `FC_COMMUNICATION_LOSS` | FC1 alive, but the FC1→FC2 communication path is cut. | `comms_link_up = false`: every packet dropped. | none |
| FM-03 | `COMMUNICATION_DEGRADED` | Intermittent packet loss; the link stays up. Not a total loss. | `comms_loss_probability = p`. | `loss_probability` |
| FM-04 | `INVALID_SENSOR_DATA` | A sensor channel delivers invalid data (out of range, NaN, extreme noise). | `sensor_corruption` + `corrupted_altitude_m` applied to the sensor chain. | `corruption`, `corrupted_altitude_m` |
| FM-05 | `ACTUATOR_DEGRADED` | Rotational actuator (main rotor) loses effectiveness. | `actuator_efficiency < 1` scales the applied command. | `efficiency` |
| FM-06 | `CONTROL_DEADLINE_MISSED` | Deadline miss in a control loop. **Documented, no injection path yet.** | — | — |
| FM-07 | `INVALID_NUMERICAL_STATE` | NaN / Infinity / invalid physics state. **Documented, no injection path yet** (NaN injection exists only through the FM-04 altitude channel). | — | — |

`sim::sil::failure_mode_implemented()` reports the honest coverage and
`make_fault_injector()` rejects unimplemented modes with
`InjectorError::UnsupportedFailureMode`.
### 2.1 Target coverage (honest component mapping)

The `sim::sil::FaultTarget` vocabulary declares more components than the
injection machinery can disturb. The factory rejects unsupported targets with
`InjectorError::UnsupportedFaultTarget` so no scenario can pretend to fault a
component that has no injection path.

| Failure mode parameter concept | `FaultTarget` | Injectable? |
| :--- | :--- | :--- |
| Sensor — altitude (`ALTITUDE`) | `SensorBarometer` | **Yes** |
| Sensor — position (`POSITION`) | — (no target) | No — the range check validates position, but no injection path exists |
| Sensor — `IMU` | `SensorImu` | No — declared, future work |
| Sensor — `GNSS` | `SensorGnss` | No — declared, future work |
| Sensor — `RPM_FEEDBACK` | `SensorRpmFeedback` | No — declared, future work |
| Actuator — `ROTATION_SYSTEM` | `ActuatorMainRotor` | **Yes** |
| Actuator — `LEFT_SERVO` | `ActuatorLeftServo` | No — declared, future work |
| Actuator — `RIGHT_SERVO` | `ActuatorRightServo` | No — declared, future work |
| Processor | `ProcessorFc1` | **Yes** (FM-01) |
| Link | `LinkFc1Fc2` | **Yes** (FM-02, FM-03) |

---

## 3. Detection Mechanisms and Detection Events

`sim::safety::DetectionEvent` — the detection outputs produced by the FC2
`HealthMonitor`. Each value names the **mechanism** that fired, never the
injected root cause.

| Detection Event | Mechanism | Latched? |
| :--- | :--- | :--- |
| `FC1_HEARTBEAT_TIMEOUT` | Heartbeat supervision: no FC1 message within `heartbeat_timeout_s` while the link is up. | Yes (reset only) |
| `COMMUNICATION_TIMEOUT` | Link supervision: the FC1→FC2 communication path reports down. | Yes (reset only) |
| `SENSOR_VALIDATION_FAILED` | Range/NaN validation of the sensor telemetry (`SensorValidationLimits`). | No (auto-clears) |
| `ACTUATOR_MISMATCH` | Sustained commanded/actual RPM mismatch (`actuator_mismatch_rpm` over `actuator_mismatch_hold_s`). | No (auto-clears) |

### 3.1 Heartbeat supervision vs watchdog (strict separation)

- **Heartbeat supervision (remote, system-level)** — implemented: FC1
  publishes periodic heartbeats, FC2 monitors their arrival
  (`FC1_HEARTBEAT_TIMEOUT`, `COMMUNICATION_TIMEOUT`; trace events
  `SUPERVISION_TIMEOUT`, `SUPERVISION_RESET`, `SUPERVISION_RECOVERY`).
- **Watchdog (local, task-level)** — *not implemented*: a local execution
  task stops responding (e.g. a future `CONTROL_TASK_STALL` detection). The
  word *watchdog* is reserved for this local task-level concept.

> **Rule**: watchdog terminology is never used for network/communication
> monitoring. The HIL bench deadline supervision (`DEADLINE_MISSED` event,
> `DeadlinePolicy`) is the HIL-domain detection mechanism associated with the
> documented FM-06 failure mode; it supervises the bench loop, not an aircraft
> control task.

---

## 4. System States and Safety Actions

### 4.1 Health states (`sim::safety::HealthState`)

| State | Meaning | Produced when |
| :--- | :--- | :--- |
| `HEALTHY` | Nominal operation. | No detection flag raised. |
| `DEGRADED` | Secondary subsystem anomaly; controllability preserved. | `SENSOR_VALIDATION_FAILED` or `ACTUATOR_MISMATCH` raised. |
| `SAFE` | Major failure; mission cannot continue normally. | `FC1_HEARTBEAT_TIMEOUT` or `COMMUNICATION_TIMEOUT` raised. |

`FAILED` was removed from the enum: no detection path ever produced it (dead
state). It can be reintroduced when an integrity-loss detection mechanism
exists.

### 4.2 Safety modes and actions (`sim::safety::SafetyMode` / `SafetyAction`)

A safety **mode** is a system state; a safety **action** is the commanded
transition that engages it (`safety_action_for()` maps one to the other).

| Safety Action | Safety Mode | Effect |
| :--- | :--- | :--- |
| `RESUME_NORMAL` | `NORMAL` | Nominal FC1 command path. |
| `ENTER_COMPENSATED` | `COMPENSATED` | Degraded mode: thrust margin (1.7×) only on proven `ACTUATOR_MISMATCH`; nominal thrust otherwise. |
| `ENTER_SAFE_MODE` | `SAFE_MODE` | Mission abort + controlled descent (irreversible). |


---

## 5. Implementation Coverage Matrix

| Failure Mode | Implemented | Detection Mechanism | Safety Response | Status / Notes |
| :--- | :---: | :--- | :--- | :--- |
| **FM-01 FC1 Unavailable** | Yes | Heartbeat supervision → `FC1_HEARTBEAT_TIMEOUT` | `ENTER_SAFE_MODE`, mission abort | Fully tested (SIL FAULT_INJECTOR-001/005, HIL FAULT_INJECTOR-001) |
| **FM-02 Communication Loss** | Yes | Link supervision → `COMMUNICATION_TIMEOUT` | `ENTER_SAFE_MODE`, mission abort | Fully tested (SIL/HIL FAULT_INJECTOR-002) |
| **FM-03 Communication Degraded** | Yes | Link supervision absorbs the loss rate; `COMMUNICATION_TIMEOUT` only if the supervision window is exceeded | Depends on absorption | Monte-Carlo coverage only (no deterministic scenario) |
| **FM-04 Invalid Sensor Data** | Yes | Range/NaN validation → `SENSOR_VALIDATION_FAILED` | `ENTER_COMPENSATED` (nominal thrust), FC1 holds last valid measurement | Altitude channel only; fully tested (SIL/HIL FAULT_INJECTOR-003) |
| **FM-05 Actuator Degraded** | Yes | Sustained RPM mismatch → `ACTUATOR_MISMATCH` | `ENTER_COMPENSATED` with 1.7× thrust margin | Main rotor only; fully tested (SIL/HIL FAULT_INJECTOR-004) |
| **FM-06 Control Deadline Missed** | No | HIL bench deadline supervision (`DEADLINE_MISSED`) exists; no SIL detection | N/A | Documented; no injection path, no RTOS task-stall model |
| **FM-07 Invalid Numerical State** | No | NaN rejected by sensor validation (FM-04 path only) | N/A | Documented; generic numerical-state guard is future work |
| **IMU / GNSS sensor failure** | No | None | N/A | Future work (targets declared, injection rejected) |
| **Servo actuator failure** | No | None | N/A | Future work (targets declared, injection rejected) |

---

## 6. Enum Inventory (classification review)

Every enum of the fault/safety vocabulary classified by concept:

| Enum | Module | Classification |
| :--- | :--- | :--- |
| `FaultDomain { SYSTEM, HIL, INFRASTRUCTURE }` | `SilFaultScenario` | Problem domain taxonomy |
| `FailureMode` | `SilFaultScenario` | **Failure Mode** (root cause) |
| `FaultTarget` | `SilFaultScenario` | Failure mode parameter (targeted component) |
| `FaultProfile` | `SilFaultScenario` | Failure mode parameter (temporality) |
| `FaultValueKind` | `SilFaultScenario` | Failure mode parameter (numeric semantic) |
| `SensorCorruptionMode` | `Telemetry` | Failure mode parameter (FM-04 corruption kind) |
| `DetectionEvent` | `HealthMonitor` | **Detection Event** (detection output) |
| `HealthState` | `HealthMonitor` | **System State** (health) |
| `SafetyMode` | `SafetyManager` | **System State** (safety) |
| `SafetyAction` | `SafetyManager` | **Safety Action** (reaction) |
| `SilEventType` / `HilEventType` | `SilEvents` / `HilEvents` | Trace event types (chain stages) |
| `EventSeverity` / `SilLogLevel` / `HilEventSeverity` | events modules | Logging verbosity |
| `MissionState` | `FlightControllerTypes` | **System State** (mission) |
| `InjectorError` | `FaultInjectors` | INFRASTRUCTURE configuration error |
| `SilError` / `ReportError` | SIL modules | INFRASTRUCTURE error |
| `HilError` | `HilRunnerTypes` | INFRASTRUCTURE / HIL-bench error |
| `ReceiveResult` / `FrameAcceptanceError` | `HilTransport` | HIL bench transport outcome |
| `DeadlinePolicy` / `ClockKind` | `HilConfig` | HIL bench configuration |
| `FailureReason` | `ValidationTypes` | Test-verdict classification (INFRASTRUCTURE) |
| `FunctionalFamily` | `FunctionalScenarios` | Test taxonomy |

### Cleanup performed

- `sim::safety::FaultDomain` renamed to `DetectionEvent`: it classified
  detection outputs, not fault domains.
- `FaultType` renamed to `FailureMode`; values renamed to explicit root causes
  (`FC1Failure` → `FC1_UNAVAILABLE`, `CommunicationLoss` →
  `FC_COMMUNICATION_LOSS`, `CommunicationLossRate` → `COMMUNICATION_DEGRADED`,
  `SensorFault` → `INVALID_SENSOR_DATA`, `ActuatorDegradation` →
  `ACTUATOR_DEGRADED`).
- `HealthState::FAILED` removed (dead state, never produced).
- `SilEventType::MessageGenerated / MessageDelivered / MessageDropped` removed
  (dead event types, never emitted; the heartbeat events carry the traffic).
- Watchdog terminology removed from the heartbeat/link supervision:
  `WatchdogTimeout` → `SupervisionTimeout`, `WatchdogKick` →
  `SupervisionReset`, `WatchdogRecovery` → `SupervisionRecovery`,
  `watchdog_triggered` → `supervision_triggered`,
  `FailureReason::WatchdogMissed` → `SupervisionMissed`.
- Typed injection events renamed to mark their injection nature:
  `SensorFault` → `SensorFaultInjected`, `ActuatorFault` →
  `ActuatorFaultInjected`.


---

## 7. Logging Contract

The structured trace must allow the unequivocal reconstruction of the safety
chain: injection → detection → classification → safety action → mission
result. Field roles:

- `FAULT_INJECTED` carries the **failure mode** (`detail`), the target
  identity, the parameters and the expected system response.
- `SUPERVISION_TIMEOUT` carries the **detection event** (`detail`).
- `FAULT_DETECTED` / `FAULT_CLASSIFIED` carry the **detection event**.
- `SAFETY_RESPONSE` carries the **safety action** (`detail`).
- `SAFETY_STATE_TRANSITION` / `MISSION_STATE_TRANSITION` carry the **system
  states** (`previous_state` / `new_state`).

Example (FM-01, JSONL trace):

```json
{"event":"FAULT_INJECTED","details":{"detail":"FC1_UNAVAILABLE","target":"fc1_primary"}}
{"event":"FC_FAILURE","details":{"detail":"FC1_UNAVAILABLE"}}
{"event":"SUPERVISION_TIMEOUT","details":{"detail":"FC1_HEARTBEAT_TIMEOUT"}}
{"event":"FAULT_DETECTED","details":{"detail":"FC1_HEARTBEAT_TIMEOUT"}}
{"event":"FAULT_CLASSIFIED","details":{"detail":"FC1_HEARTBEAT_TIMEOUT"}}
{"event":"SAFETY_RESPONSE","details":{"detail":"ENTER_SAFE_MODE"}}
{"event":"SAFETY_STATE_TRANSITION","details":{"previous_state":"NORMAL","new_state":"SAFE_MODE"}}
```

Anti-pattern (avoid): a single `WATCHDOG_TIMEOUT` event whose detail names a
root cause — it mixes the detection mechanism with the failure mode.

---

## 8. Scope Boundary

Explicitly out of scope for this refactoring phase: full FMECA execution or
matrix generation, new physical sensor/actuator hardware models, new flight
dynamics, new RTOS scheduler features, new Monte-Carlo suites. This document
only aligns the architecture and the vocabulary; the coverage matrix above is
the honest statement of what exists.

