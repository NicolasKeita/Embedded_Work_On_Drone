# Prompt Coding Agent: Refactoring Safety Architecture & Fault Taxonomy

> **Note**: Ce prompt est orienté refactoring d'architecture et de vocabulaire (préparation à la FMECA) sans ajout de fonctionnalités massives, afin d'éviter la création d'anomalies fictives.

---

# Task: Harmonize fault taxonomy, safety architecture vocabulary and failure modeling

## Context

This project is a Heliblade-like distributed flight control and safety testbed.

> **Goal**: The goal is **NOT** to implement the FMECA yet.
> 
> The goal of this task is to make the current codebase technically and semantically ready for a future FMECA by:
> - Cleaning the fault taxonomy;
> - Separating aircraft/system faults from HIL/test infrastructure faults;
> - Improving naming consistency;
> - Removing misleading abstractions;
> - Clarifying the difference between detection mechanisms, failure modes, and recovery actions;
> - Ensuring that the code vocabulary matches embedded/aerospace engineering terminology.

**Constraints:**
- Do **not** add unnecessary new features.
- Do **not** artificially increase fault coverage.
- **Only** improve architecture, naming, and existing behavior.

---

# Current Architecture Concept

The system currently contains:
- **FC1**: Primary flight controller
- **FC2**: Safety/supervision controller
- **Health Monitoring**
- **Safety Manager**
- **Fault Injector**
- **SIL/HIL simulation framework**
- **Telemetry / Event logging**

### Intended Safety Chain

```text
Failure Mode
     │
     ▼
Detection Mechanism
     │
     ▼
Diagnosis / Classification
     │
     ▼
Safety Response
     │
     ▼
Recovery / Safe Mode / Degraded Mode
```

Keep this conceptual separation intact.

---

# Main Objectives

## 1. Separate Three Categories of Problems

Currently, some faults, HIL errors, and software/tooling errors are mixed. Create a clear distinction using the following taxonomy:

### `FaultDomain`

- `SYSTEM`: Real aircraft / embedded system failures.
- `HIL`: Hardware-in-the-loop or communication test bench failures.
- `INFRASTRUCTURE`: Test framework, filesystem, CLI, or configuration errors.

> **Focus**: The main FMECA preparation should focus **only** on `SYSTEM` faults.

---

## 2. Rename and Reorganize System Faults

Create a coherent naming convention.

- Use **`FailureMode`** for actual failure causes.
- Use **`DetectionEvent`** for detection outputs.
- Use **`SafetyAction`** for reactions.

> ⚠️ **Do NOT mix these concepts.**

#### Counter-Example & Correction
* **Bad**: `WATCHDOG_TIMEOUT` (ambiguous: does it mean FC crashed, communication lost, or task stalled?)
* **Good**: Replace with explicit, domain-specific names.

---

### Proposed System Failure Modes

Adapt existing code to these concepts:

#### **FM-01: FC1 Unavailable**
- **Meaning**: Primary flight controller is no longer operational (e.g., crash, stopped execution, no heartbeat).
- **Suggested Failure Mode**: `FailureMode::FC1_UNAVAILABLE`
- **Detection Event**: `DetectionEvent::FC1_HEARTBEAT_TIMEOUT`
- *Note*: Do not call the detection mechanism itself a failure mode.

#### **FM-02: Communication Loss**
- **Meaning**: The controller is alive, but communication path is unavailable.
```text
FC1 (alive) ────── [ X ] ──────> FC2 (cannot receive messages)
```
- **Suggested Failure Mode**: `FailureMode::FC_COMMUNICATION_LOSS`
- **Detection Event**: `DetectionEvent::COMMUNICATION_TIMEOUT`

#### **FM-03: Communication Degradation**
- **Meaning**: Packet loss or intermittent failures (do not represent this as total communication loss).
- **Suggested Failure Mode**: `FailureMode::COMMUNICATION_DEGRADED`
- **Parameters**: `loss_probability`, `latency`, `jitter`

#### **FM-04: Invalid Sensor Data**
- **Meaning**: Keep sensor failure representations generic and honest. Do not pretend IMU/GNSS failures exist if they are not actually injected.
- **Suggested Failure Mode**: `FailureMode::INVALID_SENSOR_DATA`
- **Parameters**:
  ```cpp
  enum class SensorType {
      ALTITUDE,
      POSITION,
      IMU,
      GNSS,
      RPM_FEEDBACK
  };
  ```
- *Current Coverage*: Keep coverage honest (e.g., `ALTITUDE` = implemented, `IMU` = not implemented). Do not add fake implementations.

#### **FM-05: Actuator Degradation**
- **Meaning**: Current implementation mainly affects rotational actuator/RPM. Do not claim independent servo failures if unsupported.
- **Suggested Failure Mode**: `FailureMode::ACTUATOR_DEGRADED`
- **Parameters**:
  ```cpp
  enum class ActuatorType {
      ROTATION_SYSTEM,
      LEFT_SERVO,
      RIGHT_SERVO
  };
  ```
- *Note*: Only mark implemented components as supported.

#### **FM-06: Control Timing Failure**
- **Meaning**: Deadline misses in control loops (critical embedded software failure mode).
- **Suggested Failure Mode**: `FailureMode::CONTROL_DEADLINE_MISSED`
- **Detection Event**: `DetectionEvent::TASK_DEADLINE_EXCEEDED`

#### **FM-07: Invalid Numerical State**
- **Meaning**: Documented future failure mode (e.g., `NaN`, `Infinity`, invalid physics state).
- **Suggested Failure Mode**: `FailureMode::INVALID_NUMERICAL_STATE`
- *Note*: Do not necessarily implement active injection yet.

---

## 3. Separate Watchdog from Heartbeat Supervision

Currently, watchdog terminology is ambiguous. Establish a strict separation:

### Heartbeat Supervision (Remote / System-level)
- **Concept**: FC1 sends periodic heartbeats; FC2 monitors heartbeat arrival.
- **Detection**: `DetectionEvent::FC1_HEARTBEAT_TIMEOUT`

### Watchdog (Local / Task-level)
- **Concept**: A local execution task stops responding.
- **Example Future Coverage**: `CONTROL_TASK_STALL`

> 🛑 **Rule**: Do not use watchdog terminology for network/communication monitoring.

---

## 4. Improve Enums / Classes Organization

Review existing enums and look for:
- Duplicated concepts
- Enums declared but never used
- Misleading or overloaded names
- Dead states

**Examples to review**: `HealthState`, `FailureType`, `FaultType`, `ReceiveResult`, `MessageTypeError`, `WatchdogTimeout`.

For **every** enum, ask:
1. Is it a *Failure Mode*?
2. Is it a *Detection Event*?
3. Is it a *System State*?
4. Is it a *Safety Action*?

If the answer is unclear, rename or refactor it.

---

## 5. Improve Logging Vocabulary

Logs must allow unequivocal reconstruction of the safety chain sequence:

```text
Fault Injection ──> Detection ──> Decision ──> Safety Transition ──> Mission Result
```

#### Good Example
```json
{
  "event": "FAULT_INJECTED",
  "failure_mode": "FailureMode::FC1_UNAVAILABLE"
}
{
  "event": "FAULT_DETECTED",
  "detection": "DetectionEvent::FC1_HEARTBEAT_TIMEOUT"
}
{
  "event": "SAFETY_ACTION",
  "action": "SafetyAction::ENTER_SAFE_MODE"
}
```

#### Bad Example (Avoid)
```json
{
  "event": "WATCHDOG_TIMEOUT",
  "detail": "FC1_FAILURE"
}
```
*(Mixes the event detection with the underlying cause)*

---

## 6. Improve Fault Injector Architecture

The injector should inject **`FailureMode`** objects rather than low-level implementation hacks.

* **Before**: `injectWatchdogTimeout()`
* **After**: `injectFailure(FailureMode::FC1_UNAVAILABLE)`

The injector component handles the physical or simulated representation:
- **FC1 Unavailable**: Stop heartbeat generation.
- **Communication Loss**: Drop network packets.
- **Sensor Failure**: Corrupt sensor values.

---

## 7. Improve Documentation and Comments

Update inline comments and documentation to strictly use embedded safety terminology.

- **Avoid generic terms** like `"error"` when referring to specific concepts.
- **Use precise vocabulary**: *Failure Mode*, *Detection Event*, *Degraded State*, *Recovery Action*.

---

## 8. Explicitly Scope Out (Do NOT Implement Yet)

Do **NOT** implement the following during this refactoring phase:
- Full FMECA execution / matrix generation
- New physical sensors or actuator hardware models
- New physical flight dynamics simulation
- New RTOS scheduler features
- New Monte Carlo test suites

> **Scope Boundary**: This task is strictly architectural preparation and semantic alignment.

---

## 9. Expected Deliverables

### 1. Codebase Changes
- Renamed and reorganized enums/classes.
- Cleaned fault taxonomy.
- Standardized comments and logging terminology.
- Updated existing unit/integration tests to match new names.

### 2. Documentation
Create or update `docs/safety/fault_taxonomy.md` containing:
- Fault domains
- Failure modes mapping
- Detection mechanisms
- Safety actions
- Implementation coverage matrix

#### Coverage Matrix Template

| Failure Mode | Implemented | Detection Mechanism | Safety Response | Status / Notes |
| :--- | :---: | :--- | :--- | :--- |
| **FC1 Unavailable** | Yes | Heartbeat Timeout | Safe Mode | Fully tested |
| **Communication Loss** | Yes | Comm Timeout | Safe Mode | Fully tested |
| **Invalid Altitude Sensor** | Yes | Health Monitor | Degraded Mode | Single sensor |
| **IMU Failure** | No | None | N/A | Future work |
| **Servo Failure** | No | None | N/A | Future work |

---

# Final Validation Checklist

Before marking the task complete, verify that:
- [ ] No fault name mixes root cause and detection event.
- [ ] No HIL protocol/bench failure is miscategorized as an aircraft failure.
- [ ] No future/unimplemented fault is presented as currently supported.
- [ ] All existing test suites pass without regression.
- [ ] SIL/HIL simulation behavior remains functionally identical.

---
*The objective is not to write more code, but to establish a clean, professional engineering model suitable for formal embedded safety analysis (FMECA).*
