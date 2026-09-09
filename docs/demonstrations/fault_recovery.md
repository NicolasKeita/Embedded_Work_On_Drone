# Demonstration 1 — Graceful Degradation and Recovery

> Fault: `INVALID_SENSOR_DATA` (temporary, recoverable). Target: SIL **or** HIL
> `FAULT_INJECTOR-003`. This demo uses the **HIL** scenario because its fault
> window (t = 5 → 25 s) leaves time to observe full recovery before the 30 s run
> ends.
> Related: [validation matrix](../validation/test_matrix.md) ·
> [FMECA FM-04](../fmeca/fmeca.md).

## 1. Scenario

The aircraft is on its nominal station-keeping mission (target altitude 10 m).
At t = 5.0 s the altitude/barometer sensor is corrupted to an out-of-range
value (99999 m) for 20 s. The fault is *temporary*: it clears at t = 25.0 s.

## 2. Expected flow

```text
NORMAL (HEALTHY)
   ↓  fault injected at t = 5.0 s (INVALID_SENSOR_DATA)
SENSOR_VALIDATION_FAILED (≤ 500 ms)
   ↓
DEGRADED  →  COMPENSATED   (hold last valid measurement, nominal thrust)
   ↓  fault window ends at t = 25.0 s
sensor data valid again → SENSOR_VALIDATION_FAILED flag clears
   ↓
HEALTHY  →  NORMAL   (RECOVERY_END / SUPERVISION_RECOVERY)
   ↓
mission continues / completes (not aborted)
```

## 3. Detection mechanism

`HealthMonitor::update_sensor_flags` re-validates `SensorTelemetry` every step
via `validate()` (range/NaN). On corruption it raises
`SENSOR_VALIDATION_FAILED`; `HealthState` becomes `DEGRADED` and the
`SafetyManager` engages `COMPENSATED`. Because **no actuator mismatch** is
detected, the thrust margin stays nominal (1.0) — the controller keeps flying
on the **last valid altitude measurement** (`FlightController` holds
`fc1_view` while validation fails). When the window ends, validation passes
again, the detection flag clears, `HealthState` returns `HEALTHY` and the
`SafetyManager` resumes `NORMAL` (`RECOVERY_END` is recorded).

## 4. Representative output

> Representative/expected output of `HIL_RUNNER --scenario FAULT_INJECTOR-003`,
> derived from the deterministic engine and the reused SIL-baseline detection
> chain. **Not captured live** in this report's environment (no C++23 toolchain).
> Run the command on the configured toolchain to capture the real artifact.

Telemetry (sensor stream; ~1 s cadence; columns `t  x  y  z  vx  vy  vz  pitch roll rpm`):

```text
   t(s)     x(m)     y(m)     z(m)   vx  vy  vz   pitch roll  rpm
[MISSION] t = 0.77 s -> CLIMB
[MISSION] t = 2.89 s -> STATION_KEEPING
   5.00     0.00     0.00    10.00    0   0   0    0.0  0.0   815   (hold last valid z)
[FAULT]  t = 5.00 s -> FAULT_INJECTED (INVALID_SENSOR_DATA)
[SAFETY] t = 5.05 s -> SAFETY_STATE_TRANSITION (NORMAL -> COMPENSATED)
   6.00     0.00     0.00    10.00    0   0   0    0.0  0.0   815   (degraded, holding)
  ...
  25.00     0.00     0.00    10.00    0   0   0    0.0  0.0   815
[FAULT]  t = 25.00 s -> FAULT_CLEARED
[SAFETY] t = 25.05 s -> SAFETY_STATE_TRANSITION (COMPENSATED -> NORMAL)
  26.00     0.00     0.00    10.00    0   0   0    0.0  0.0   815   (recovered)
  30.00     0.00     0.00    10.00    0   0   0    0.0  0.0   815
```

Key event timeline:

```text
5.00  FAULT_INJECTED         (INVALID_SENSOR_DATA)
5.05  FAULT_DETECTED         (SENSOR_VALIDATION_FAILED)
5.05  SAFETY_STATE_TRANSITION (NORMAL -> COMPENSATED)
25.00 FAULT_CLEARED          (window ended)
25.05 RECOVERY_END           (health restored)
25.05 SUPERVISION_RECOVERY
25.05 SAFETY_STATE_TRANSITION (COMPENSATED -> NORMAL)
```

Mission result:

```text
Mission success   : YES      (mission not aborted — degraded fault)
Final state       : COMPLETE (or STATION_KEEPING / continues)
Final safety mode : NORMAL    (recovered)
Final health      : HEALTHY
Fault detected    : YES
Test verdict      : PASS
  > degrading fault compensated, mission pursued (recovery observed)
```

> Note on the reported `z`: during the fault the sensor path reports a
> corrupted altitude, but FC1 holds the last *valid* measurement, so the
> *commanded* behaviour stays centered on 10 m; the *ground-truth* altitude
> (recorded separately) remains ~10 m. The exact recovery timestamp depends on
> the first post-clear validation step.

## 5. Effect on telemetry

* Commanded altitude/position remain centered on the target (hold-last-valid).
* Ground-truth altitude stays ~10 m (no physical excursion from a sensor fault).
* Safety/safety-state columns transition `NORMAL → COMPENSATED → NORMAL`.

## 6. Reproduce

```text
HIL_RUNNER --scenario FAULT_INJECTOR-003            # HIL, recovery within 30 s
SIL_RUNNER --scenario FAULT_INJECTOR-003            # SIL (window 20→30 s; recovery at boundary)
HIL_RUNNER --scenario FAULT_INJECTOR-003   # real-time HIL execution
```

## 7. Limitations of this demo

* Only the **altitude/barometer** channel is injectable (IMU/GNSS/RPM faults
  are declared but have no injection path — FM-04 §11).
* Detection is range/NaN validation: an in-range-but-wrong value (silent drift)
  is **not** detected.
* The SIL window (20→30 s) clears exactly at run end, so the HIL scenario is the
  cleaner demonstration of recovery.
