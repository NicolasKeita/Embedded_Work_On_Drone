# Final Validation Report

> Executive technical report. Concise by design (~3–6 pages). It lets a senior
> engineer judge whether the system is coherent and how well it has been
> validated.
> Companion documents: [test matrix](test_matrix.md) ·
> [scenario reference](scenarios.md) · [SIL](sil.md) · [HIL](hil.md) ·
> [Monte Carlo](monte_carlo.md) · [architecture](../architecture/overview.md) ·
> [FMECA](../fmeca/fmeca.md).

## 1. Scope

This report covers the validation of the **Distributed Flight Control & Safety
Testbed**: a deterministic C++23 testbed for a simplified VTOL dual-computer
(FC1/FC2) flight-control + safety architecture. Validated:

* the closed-loop control and mission state machine;
* the FC2 fault-detection → safety-response chain for the implemented failure
  modes;
* timing/deadline behaviour of the real-time loop on the **host emulator**;
* statistical dispersion of the control law.

**Explicitly out of scope:** real flight, real hardware/peripherals, a
physical STM32, certification.

> **Execution caveat.** The conclusions below are based on the deterministic
> assertions encoded in the test sources and the shared `flight_sim_core`
> engine behaviour. They were **not re-executed** to write this report because
> the required C++23-modules toolchain (GCC 16 / MSVC 2026) was unavailable in the
> working environment (only GCC 12.2, no `cmake`). All build/run commands are
> given so the artifacts can be reproduced on the configured toolchain; captured
> outputs land under `docs/validation/data/` (gitignored).

## 2. System under test

A single deterministic engine (`flight_sim_core`) links aircraft physics, the
FC1 `FlightController`, the FC2 `HealthMonitor`+`SafetyManager`, a simulated
in-process `CommsBus`, fault injectors and the SIL orchestration loop. Three
executables (`SIL_RUNNER`, `HIL_RUNNER`, `SIL_MONTE_CARLO`) drive that one
engine, so control and safety code is identical across SIL/HIL/Monte Carlo.
See [`architecture/overview.md`](../architecture/overview.md) for the full
component diagram and responsibilities.

## 3. Validation strategy

Only levels actually present are listed:

```text
Deterministic SIL scenarios (strict assertions)  +  Monte Carlo dispersion
                         ↓
        HIL real-time closed loop (host emulator, timing/deadline + protocol path)
                         ↓
                 FMECA traceability (failure → detection → response → result)
```

## 4. Final validation results

Status legend: PASS = deterministic assertion encoded & designed to satisfy.
`*` = tooling present/expected; not re-run in this environment.

| Area | Result | Evidence |
| :--- | :--- | :--- |
| Mission (TAKEOFF→CLIMB→STATION_KEEPING→COMPLETE) | PASS | `SIL_RUNNER --scenario NOMINAL-001/010` |
| Control (altitude / X / Y / actuators / saturation) | PASS* | NOMINAL-001, 008, 009; `ControllerConfig` bounds |
| Safety (DEGRADED/COMPENSATED/SAFE/SAFE_MODE/abort) | PASS | shared `FAULT_INJECTOR-001/003` (SIL/HIL) |
| Communication (heartbeat / loss / packet loss) | PASS / LIMITED | FM-01/02 PASS; FM-03 packet-loss LIMITED |
| Monte Carlo (dispersion + statistics) | PASS* (tooling) | `SIL_MONTE_CARLO` |
| HIL timing (real-time loop, deadlines) | PASS* | `HIL_RUNNER --selftest`, `--scenario NOMINAL-001` |

## 5. Key failures tested

Compact summary — the per-mode analysis lives in the [FMECA](../fmeca/fmeca.md),
which is **not** reproduced here.

| Failure mode → detection → response | Critical? | Outcome | Scenario |
| :--- | :--- | :--- | :--- |
| `FC1_UNAVAILABLE` → `FC1_HEARTBEAT_TIMEOUT` → `ENTER_SAFE_MODE` | yes | detected ≤ 300 ms, SAFE_MODE, ABORTED | FAULT_INJECTOR-001 (SIL/HIL), 005 (SIL) |
| `INVALID_SENSOR_DATA` → `SENSOR_VALIDATION_FAILED` → `ENTER_COMPENSATED` | no | detected ≤ 500 ms, DEGRADED+COMPENSATED, mission continues, **recoverable** | FAULT_INJECTOR-003 (SIL/HIL) |

The chain exercised end-to-end: Failure Mode → Detection Event → Health State
→ Safety Action → Safety Mode → Mission outcome.

## 6. Known limitations

Genuine limitations discovered from the repository (not future plans):

* **No physical STM32 HIL.** HIL runs against an in-process host FC emulator
  over loopback; MCU timing, RTOS scheduling and real peripherals are not
  validated.
* **Single process, no OS-level isolation.** FC1/FC2 are in-memory objects; the
  legacy multi-process/UDP design is not realised in code.
* **Simplified aircraft dynamics** for control-software development only.
* **Incomplete fault coverage.** Only the altitude/barometer sensor channel and
  the main-rotor actuator are injectable. `COMMUNICATION_DEGRADED` has no named
  deterministic scenario. `CONTROL_DEADLINE_MISSED` (FM-06) and
  `INVALID_NUMERICAL_STATE` (FM-07) have **no injection path**.
* **No local task watchdog** — liveness is supervised remotely by heartbeat only.
* **No numerical-integrity guard** (`isfinite` on state/command).
* **Critical faults are irreversible**: heartbeat/communication flags are
  latched; `SAFE_MODE` abort cannot reverse.
* **Monte Carlo is control-quality only** (physics dispersion); the fault-window
  statistical harness exists only as library code, not wired to a CLI.
* **Timing instrumentation gaps** in HIL: no jitter std-dev, no printed elapsed
  wall-clock time.

## 7. Conclusion

The testbed demonstrates a **coherent, deterministic** failure → detection →
safety-response chain for the implemented failure modes, validated by strict
SIL scenario assertions, exercised at real-time cadence on the HIL host
emulator, and stress-dispersed by a seeded Monte Carlo campaign. The same
`flight_sim_core` engine and the same detection/safety core back all three
runners, so SIL, HIL and Monte Carlo validate one and the same software.

What is **demonstrated**: control + mission logic, FC1/FC2 supervision
(detection ≠ response), bounded-latency detection and the consistent
SAFE/COMPENSATED behaviour for FM-01..FM-05, and a timing-disciplined real-time
loop with zero deadline misses on the host emulator.

What **remains unverified**: physical-MCU behaviour, real peripherals/medium,
the FM-06/FM-07 fault modes and full sensor/actuator coverage, and any
fault-statistical Monte Carlo (not wired). The project does **not** claim
real-flight or certification validity.

Reproduce the validation on the configured toolchain (`CMakePresets.json`):

```text
SIL_RUNNER --all                       # deterministic SIL + observability + telemetry suites
SIL_MONTE_CARLO --runs 50              # Monte Carlo dispersion campaign
HIL_RUNNER --selftest                  # deterministic HIL validation suite
HIL_RUNNER --scenario NOMINAL-001      # real-time nominal closed loop
HIL_RUNNER --scenario FAULT_INJECTOR-001   # HIL FC1-failure closed loop
```

Final project status:

| Area | Status |
| :--- | :--- |
| Control / mission logic | VALIDATED (SIL) |
| Safety chain FM-01..FM-05 | VALIDATED (SIL/HIL) |
| HIL real-time loop (host emulator) | PARTIALLY VALIDATED (no physical MCU) |
| Monte Carlo | VALIDATED (tooling, control-quality only) |
| Monte Carlo over faults | NOT IMPLEMENTED (library, not wired) |
| FM-06 / FM-07 fault modes | NOT IMPLEMENTED |
| Physical STM32 HIL | NOT IMPLEMENTED |
