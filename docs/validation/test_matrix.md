# Final Validation Matrix

> **What exactly has been validated.** This matrix maps each capability to its
> requirement, validation method, expected result, status, and the
> executable/scenario + document that evidence it. The per-scenario detail lives
> in the [scenario reference](scenarios.md).

> **Execution note (read this first).** Every status below is derived from the
> **deterministic assertions encoded in the test sources** and the shared
> `flight_sim_core` engine behavior — not from a one-off manual run. They were
> **not re-executed in this report's environment** because the required
> C++23-modules toolchain (GCC 16 / MSVC 2026, see `CMakePresets.json`) was not
> available (only GCC 12.2, no `cmake`). The build/run commands in each section
> and in the [README](../../README.md) reproduce them on the configured
> toolchain and emit artifacts under `docs/validation/data/` (gitignored).
> Status `PASS` therefore means *"a deterministic test asserts this and the
> shared engine is designed to satisfy it"*; `NOT IMPLEMENTED` means there is
> no injection path at all.

Status legend: **PASS** = deterministic assertion present · **LIMITED** =
partially covered · **NOT IMPLEMENTED** = no injection/detection path · **N/A** =
not applicable.

## Mission

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| MIS-01 | Progressive spin-up and takeoff transition | SIL deterministic | 3 s SPIN_UP, then 12 s smoothstep to 0.617 m/s before CLIMB | PASS | `SIL_RUNNER --scenario NOMINAL-012` | [sil.md](sil.md) |
| MIS-02 | CLIMB to target altitude (10 m) | SIL deterministic | altitude loop drives RPM; transition to STATION_KEEPING within `altitude_tolerance_m` (0.5 m) | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| MIS-03 | STATION_KEEPING holds zone for `station_hold_seconds` (5 s) | SIL deterministic | hold timer accumulates inside zone; mission COMPLETE | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| MIS-04 | COMPLETE terminal state on success | SIL deterministic | `final_state == COMPLETE`, `mission_success == true` | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| MIS-05 | Full mission TAKEOFF→CLIMB→STATION_KEEPING→COMPLETE | SIL deterministic (autonomous) | reaches COMPLETE with NORMAL/HEALTHY | PASS | `SIL_RUNNER --scenario NOMINAL-010` | [sil.md](sil.md) |

## Control

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| CTL-01 | Altitude control (altitude PID → wing RPM) | SIL deterministic | reaches 10 m; `max_altitude_error_m ≤ 10.5 m` | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| CTL-02 | X position control (position PID → tilt → servo) | SIL deterministic (autonomous) | x = 20 → 0 (cascaded X) | PASS | `SIL_RUNNER --scenario NOMINAL-008` | [sil.md](sil.md) |
| CTL-03 | Y position control (position PID → tilt → servo) | SIL deterministic (autonomous) | y = −15 → 0 (cascaded Y) | PASS | `SIL_RUNNER --scenario NOMINAL-009` | [sil.md](sil.md) |
| CTL-04 | Actuator command generation (RPM + L/R servo mix) | SIL deterministic | pitch from servo mean, roll from differential | PASS | `SIL_RUNNER --scenario NOMINAL-005/006/007` | [sil.md](sil.md) |
| CTL-05 | Controller bounds / saturation | Code inspection | RPM clamped to `[min_rpm, max_rpm]`; integral limited to `max_integral_rpm` | PASS | `FlightController` / `ControllerConfig` | [overview](../architecture/overview.md) |

## Communication (FC1 ↔ FC2)

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| COM-01 | Heartbeat publication while FC1 alive | SIL deterministic | one sequenced message/step; `CommsStats` recorded | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| COM-02 | Heartbeat timeout detection | SIL/HIL deterministic | `FC1_HEARTBEAT_TIMEOUT` within configured timeout (100 ms) | PASS | `FAULT_INJECTOR-001` (SIL+HIL) | [sil.md](sil.md) |
| COM-03 | Communication loss (link cut) | Component-level deterministic test | `COMMUNICATION_TIMEOUT` ≤ 300 ms → SAFE_MODE, ABORTED | PASS | No named functional scenario | [sil.md](sil.md) |
| COM-04 | Packet loss (degraded link) | Injector/CommsBus layer | `comms_loss_probability = p` drops packets; supervision by heartbeat window | LIMITED | `CommsBus::set_link` (FM-03) | [FMECA](../fmeca/fmeca.md) |
| COM-05 | Communication loss then recovery | — | **NOT IMPLEMENTED**: heartbeat/comm flags are latched; once SAFE_MODE engages the abort is irreversible | N/A | — | [sil.md](sil.md) |

## Safety

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| SAF-01 | DEGRADED health state on a degrading fault | SIL/HIL deterministic | `HealthState == DEGRADED` reached on sensor/actuator fault | PASS | `FAULT_INJECTOR-003/004` (SIL+HIL) | [sil.md](sil.md) |
| SAF-02 | COMPENSATED safety mode (thrust margin) | SIL/HIL deterministic | `SafetyMode == COMPENSATED`; actuator fault → 1.7× margin | PASS | `FAULT_INJECTOR-003/004` | [sil.md](sil.md) |
| SAF-03 | SAFE health state on a critical fault | SIL/HIL deterministic | `HealthState == SAFE` on FC1/comm failure | PASS | `FAULT_INJECTOR-001/002` | [sil.md](sil.md) |
| SAF-04 | SAFE_MODE engaged (conservative, irreversible) | SIL/HIL deterministic | `SafetyMode == SAFE_MODE`; controlled descent by `safe_descent_rpm_rate` | PASS | `FAULT_INJECTOR-001/002/005` | [sil.md](sil.md) |
| SAF-05 | Mission abort on critical fault | SIL/HIL deterministic | `final_state == ABORTED`; verdict PASS with mission not successful | PASS | `FAULT_INJECTOR-001/002/005` | [sil.md](sil.md) |
| SAF-06 | Recovery to NORMAL after a recoverable fault | SIL deterministic (temporary sensor fault) | sensor flag clears on valid data → HEALTHY → NORMAL; RECOVERY_END recorded | PASS | `FAULT_INJECTOR-003` (HIL window 5→25 s) | [demos](../demonstrations/fault_recovery.md) |
| SAF-07 | `FAILED` health state | — | **NOT IMPLEMENTED**: removed from `HealthState` (dead state); to be reintroduced with an integrity-detection mechanism | N/A | — | [FMECA](../fmeca/fmeca.md) |

## Fault injection

| Test ID | Failure mode | Injection point / time | Expected detection | Expected safety state | Expected mission result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| FI-01 | `FC1_UNAVAILABLE` | FC1 stop at t = 20.0 s (SIL) / t = 5.0 s (HIL) | `FC1_HEARTBEAT_TIMEOUT` ≤ 300 ms | SAFE | ABORTED | PASS | `FAULT_INJECTOR-001` (SIL/HIL) | [FMECA FM-01](../fmeca/fmeca.md) |
| FI-03 | `COMMUNICATION_DEGRADED` | `comms_loss_probability = p` | persisted/aggravated → heartbeat timeout | NORMAL or SAFE | continues or aborts | LIMITED | `CommsBus::set_link` (no named scenario) | [FMECA FM-03](../fmeca/fmeca.md) |
| FI-04 | `INVALID_SENSOR_DATA` | altitude corruption (out of range / NaN), t = 20 s / 10 s (SIL), t = 5 s / 20 s (HIL) | `SENSOR_VALIDATION_FAILED` ≤ 500 ms | DEGRADED → COMPENSATED | continues (not aborted) | PASS | `FAULT_INJECTOR-003` (SIL/HIL) | [FMECA FM-04](../fmeca/fmeca.md) |
| FI-07 | `CONTROL_DEADLINE_MISSED` | — | — | — | — | **NOT IMPLEMENTED** | — | [FMECA FM-06](../fmeca/fmeca.md) |
| FI-08 | `INVALID_NUMERICAL_STATE` | — | — | — | — | **NOT IMPLEMENTED** | — | [FMECA FM-07](../fmeca/fmeca.md) |
| FI-09 | IMU / GNSS / RPM / individual-servo faults | declared targets | — | — | — | **NOT IMPLEMENTED** (no injection path) | — | [FMECA §11](../fmeca/fmeca.md) |

## SIL

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable / scenario | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| SIL-01 | Deterministic shared scenarios | 3 SIL scenarios (NOMINAL-001 + two fault scenarios) | each meets its assertions | PASS | `SIL_RUNNER --all` | [sil.md](sil.md) |
| SIL-02 | Normal mission (no fault) | NOMINAL-001 | COMPLETE / NORMAL / HEALTHY, `max_alt_err ≤ 10.5 m` | PASS | `SIL_RUNNER --scenario NOMINAL-001` | [sil.md](sil.md) |
| SIL-03 | Fault detection | each shared fault scenario | correct `DetectionEvent`, bounded latency | PASS | `FAULT_INJECTOR-001/003` | [sil.md](sil.md) |
| SIL-04 | Safety response | each shared fault scenario | SAFE_MODE/COMPENSATED consistent with mode | PASS | `FAULT_INJECTOR-001/003` | [sil.md](sil.md) |
| SIL-05 | Mission result | each scenario | COMPLETE (nominal) / ABORTED (critical) / continues (degraded) | PASS | `--all` | [sil.md](sil.md) |
| SIL-06 | Observability suite | structured events/trace | event ordering, heartbeat/dropped/comms/fault-metadata/logging | PASS | `SilObservability` suite (`--all`) | [sil.md](sil.md) |

## Monte Carlo

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| MC-01 | Repeated stochastic campaigns | `SIL_MONTE_CARLO` dispersion campaign | N runs complete; per-run PASS/FAIL | PASS (tooling) | `SIL_MONTE_CARLO --runs N` | [monte_carlo.md](monte_carlo.md) |
| MC-02 | Pass/fail statistics | `print_summary` | total/passed/failed counts + pass rate | PASS (tooling) | `SIL_MONTE_CARLO` | [monte_carlo.md](monte_carlo.md) |
| MC-03 | Robustness / sensitivity exploration | dispersed aircraft/env/sensor inputs | overshoot/settling/steady-state/max-accel statistics | PASS (tooling) | `SIL_MONTE_CARLO -v` | [monte_carlo.md](monte_carlo.md) |
| MC-04 | Deterministic seed / reproducibility | seeded `std::mt19937_64` | same seed → same dispersion + seed export | PASS (tooling) | `--seed N` | [monte_carlo.md](monte_carlo.md) |
| MC-05 | Fault-window statistical campaign | library `FlightControlValidation::MonteCarloRunner` | — | **NOT WIRED** (library only, no CLI) | — | [monte_carlo.md](monte_carlo.md) |

> The Monte Carlo executable (`SIL_MONTE_CARLO`) runs a **physics-dispersion**
> campaign over autonomous scenarios and measures *control-quality* metrics —
> it does not inject faults. A fault-window Monte Carlo exists as library code
> (`sim::sil::validation::MonteCarloRunner`) but is not imported by any
> executable; see [monte_carlo.md](monte_carlo.md).

## HIL

## FMECA traceability

Compact mapping (the detailed analysis is in the [FMECA](../fmeca/fmeca.md);
this matrix does **not** reproduce it):

| Failure mode | Failure mode name | Primary validation | Status |
| :--- | :--- | :--- | :--- |
| FM-01 | `FC1_UNAVAILABLE` | SIL (`FAULT_INJECTOR-001`, `005`) · HIL (`FAULT_INJECTOR-001`) | PASS |
| FM-02 | `FC_COMMUNICATION_LOSS` | Component-level tests; no named scenario | PASS |
| FM-03 | `COMMUNICATION_DEGRADED` | CommsBus packet-loss layer; no named deterministic scenario | LIMITED |
| FM-04 | `INVALID_SENSOR_DATA` | SIL · HIL (`FAULT_INJECTOR-003`) | PASS |
| FM-05 | `ACTUATOR_DEGRADED` | Component-level tests; no named scenario | PASS |
| FM-06 | `CONTROL_DEADLINE_MISSED` | future timing-fault test | **NOT IMPLEMENTED** |
| FM-07 | `INVALID_NUMERICAL_STATE` | future numerical-integrity test | **NOT IMPLEMENTED** |

End-to-end traceability chain (kept practical for the major modes):

```text
Requirement / mission behavior
   → Architecture (overview.md)
      → Implementation (flight_sim_core)
         → Failure mode (FMECA)
            → Detection (HealthMonitor)
               → Safety response (SafetyManager)
                  → Validation scenario (SIL/HIL)
                     → Observed result (this matrix)
```

| Test ID | Requirement / behavior | Method | Expected result | Status | Executable | Ref |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| HIL-01 | Real-time closed-loop execution | `HIL_RUNNER` (wall-clock paced, 100 Hz) | runs `duration/dt` steps (30 s / 10 ms = 3000) | PASS | `HIL_RUNNER --scenario NOMINAL-001` | [hil.md](hil.md) |
| HIL-02 | Timing / deadline monitoring | `HilTimingStats` | deadline misses, max/mean step, round-trip, max lateness | PASS | `--scenario NOMINAL-001` | [hil.md](hil.md) |
| HIL-03 | Target interaction (loopback) | `--interface loopback` | SensorPacket/ActuatorPacket exchange over in-process channel | PASS | `--list` / `--scenario` | [hil.md](hil.md) |
| HIL-04 | Telemetry | dual sensor/truth streams | 1 Hz human-readable table + structured events | PASS | `--scenario NOMINAL-001` | [hil.md](hil.md) |
| HIL-05 | Shared fault injection through the HIL path | host-side injection → reused safety core | same detection/action as SIL baseline | PASS | `FAULT_INJECTOR-001/003` | [hil.md](hil.md) |
| HIL-06 | Physical-MCU (STM32) HIL | real target | — | **NOT IMPLEMENTED** | — | [hil.md](hil.md) |
| HIL-07 | Deterministic selftest suite | `--selftest` (runner/timing/protocol/data/faults) | all HIL tests pass | PASS | `HIL_RUNNER --selftest` | [hil.md](hil.md) |
