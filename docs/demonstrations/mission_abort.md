# Demonstration 2 — Mission Abort on Critical Failure

> Fault: `FC1_UNAVAILABLE` (critical, irreversible). Target: `FAULT_INJECTOR-001`
> (SIL @ t = 20 s, HIL @ t = 5 s). This demo uses the **HIL** scenario so the full
> controlled descent to the ground is observable within the 30 s run.
> Related: [validation matrix](../validation/test_matrix.md) ·
> [scenario reference](../validation/scenarios.md) · [FMECA FM-01](../fmeca/fmeca.md).

## 1. Initial state

The aircraft reaches its target (10 m) and enters station keeping:

```text
TAKEOFF → CLIMB (t ≈ 0.77 s) → STATION_KEEPING (t ≈ 2.89 s)
safety NORMAL, health HEALTHY, heartbeat published every step
```

## 2. Injected failure

At t = 5.0 s the primary flight controller becomes unavailable
(`fc1_alive = false`): heartbeat and command emission stop. The link itself
stays up, so this is a **heartbeat supervision** event, not a communication loss.

## 3. Detection → response

```text
MISSION (STATION_KEEPING)
   ↓  FC1 failure injected at t = 5.0 s
FC1_FAILURE / FAULT_INJECTED (FC1_UNAVAILABLE)
   ↓  heartbeat supervision timeout (100 ms)
FC1_HEARTBEAT_TIMEOUT  (FAULT_DETECTED)
   ↓
HEALTHY → SAFE  ·  SAFETY_STATE_TRANSITION (NORMAL → SAFE_MODE)
ENTER_SAFE_MODE
   ↓
MISSION_ABORTED  ·  controlled descent (safe_descent_rpm_rate = 4000 rpm/s)
   ↓
aircraft reaches the ground (z ≈ 0, rpm ≈ 0) → HIL_RUN_END
```

## 4. Detection latency

The heartbeat timeout window is 100 ms (`HilConfig::heartbeat_timeout_s`), so
detection occurs one step after FC1 goes silent at t ≈ 5.10 s:
**detection latency ≈ 0.10 s** (≤ 300 ms scenario bound; self-test asserts ≤
300 ms and response ≤ 200 ms).

## 5. Resulting states

* Mission state: `ABORTED` (set when `SAFE_MODE` engages).
* Safety mode: `SAFE_MODE` (conservative, **irreversible** — heartbeat flag is
  latched).
* Health: `SAFE`.
* Actuator: FC1 command replaced by a controlled descent
  (`safe_rpm = max(0, last_rpm − rate×dt)`, servos zeroed) until the aircraft is
  on the ground.

## 6. Representative output

> Representative/expected output of `HIL_RUNNER --scenario FAULT_INJECTOR-001`,
> derived from the deterministic engine and the reused SIL-baseline supervision
> chain. **Not captured live** in this report's environment (no C++23 toolchain).
> Run the command on the configured toolchain to capture the real artifact.

Telemetry + key events (interleaved, ~1 s cadence):

```text
[HIL] t = 0.00 s -> HIL_RUN_START
[MISSION] t = 0.77 s -> CLIMB
[MISSION] t = 2.89 s -> STATION_KEEPING
   5.00     0.00     0.00    10.00    0.00  0.00   0.00   0.0  0.0   815
[FAULT] t = 5.00 s -> FAULT_INJECTED (FC1_UNAVAILABLE)
[FAULT] t = 5.00 s -> FC1_FAILURE (FC1_UNAVAILABLE)
[SAFETY] t = 5.10 s -> HEARTBEAT_TIMEOUT (FC1_HEARTBEAT_TIMEOUT)
[FAULT] t = 5.10 s -> FAULT_DETECTED (FC1_HEARTBEAT_TIMEOUT)
[SAFETY] t = 5.10 s -> SAFETY_STATE_TRANSITION (NORMAL -> SAFE_MODE)
[MISSION] t = 5.10 s -> MISSION_ABORTED
   6.00     0.00     0.00     9.60    0.00  0.00  -0.40   0.0  0.0   774   (descending)
  10.00     0.00     0.00     ~3.5    0.00  0.00  <0     0.0  0.0   ~520
  ...
  30.00     0.00     0.00     0.00    0.00  0.00   0.00   0.0  0.0     0   (on the ground)
[HIL] t = 30.00 s -> HIL_RUN_END
```

Mission result:

```text
Mission success       : NO
Final mission state   : ABORTED
Final safety mode     : SAFE_MODE
Final health          : SAFE
Fault detected        : YES
Detection latency     : 0.1000 s
Test verdict          : PASS
  > fault detected and SAFE_MODE engaged within the required limits
```

> Degradation numbers (intermediate z/rpm) are indicative of the controlled
  descent profile; capture the exact curve by running the scenario.

## 7. Telemetry evidence

* `z` and `rpm` decrease monotonically after t = 5.10 s (controlled descent).
* `vz < 0` once the descent starts; the aircraft settles to `z ≈ 0, rpm ≈ 0`.
* The mission state switches `STATION_KEEPING → ABORTED` at the safety
  transition; it never reaches `COMPLETE`.

## 8. Relevant log events

```text
FAULT_INJECTED (FC1_UNAVAILABLE)        # injection-side observable
FC1_FAILURE                             # FC1 stopped publishing
FAULT_DETECTED (FC1_HEARTBEAT_TIMEOUT)  # FC2 supervision output
HEARTBEAT_TIMEOUT                       # FC2 heartbeat supervision
SAFETY_STATE_TRANSITION (NORMAL -> SAFE_MODE)
SAFETY_STATE_TRANSITION  [safety response engaged]
MISSION_ABORTED
```

## 9. Reproduce

```text
HIL_RUNNER --scenario FAULT_INJECTOR-001        # HIL, full descent to ground
SIL_RUNNER --scenario FAULT_INJECTOR-001        # SIL (injection at t = 20 s)
```

## 10. Limitations of this demo

* FC2 does **not** take over control (`hot failover` is not implemented); the
  safety reaction is an abort + controlled descent, not FC1 replacement.
* `SAFE_MODE` is **irreversible** — there is no return to nominal once a
  heartbeat/communication timeout has latched.
* This validates the supervision/detection/response chain, not real hardware.
