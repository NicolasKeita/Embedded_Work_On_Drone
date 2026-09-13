# Software-in-the-Loop (SIL)

> **What SIL means in this project and what it proves.**
> Related: [scenario reference](scenarios.md) · [test matrix](test_matrix.md) · [architecture overview](../architecture/overview.md).

## 1. Purpose

SIL (`SIL_RUNNER`) runs the **control and safety software** (FC1
`FlightController`, FC2 `HealthMonitor` + `SafetyManager`) against the
**software aircraft simulation**, at maximum CPU speed, with no physical MCU.
It proves the *logic* of the closed loop, the mission state machine, the fault
detection/action chain and the deterministic reproducibility of every scenario
— not any real-time or hardware behavior.

## 2. Execution architecture

```text
Aircraft simulation (Aircraft)        ── physics ground truth
        ↓
Sensors (Telemetry)                   ── SensorTelemetry; corruption + validation
        ↓
Flight Controller (FC1)                ── cascaded PID + mission FSM; heartbeat
        ↓
Actuators                              ── efficiency + thrust margin; integrate
        ↓
Aircraft simulation                    ── next physics state

FC1 ──heartbeat──► CommsBus ──► FC2 (HealthMonitor → SafetyManager) ──SafetyCommand──► FC1
```

* **Fault injection** happens in `apply_injectors`: the environment is reset to
  nominal each step and re-injected by active injectors, so *temporary* faults
  are genuinely cleared when their window ends.
* **Safety monitoring** happens in `update_monitoring`:
  `HealthMonitor.evaluate()` → `SafetyManager.update()` → detection/safety
  events recorded.
* **Telemetry** is sampled at 20 Hz (dual: sensor-observed + physics
  ground-truth); human-readable table at the configured period.
* **Events** are recorded to a structured `SilTrace` (lifecycle, fault,
  supervision, safety, recovery). Logging is purely observational: it never
  alters the run.

Per-step pipeline (fixed `dt = 0.01 s`, 100 Hz, duration 30 s):

```text
apply_injectors → update_fc1 → update_monitoring → apply_actuators → update_metrics
```

## 3. Deterministic scenarios

The SIL suite (`Tests/Sil/SilScenarios*`) runs six deterministic scenarios. Each
embeds explicit `runner.check(...)` assertions that encode the expected result;
the table below reflects those assertions (status = deterministic PASS).

| Scenario | Fault | Injection point / time | Expected detection | Expected safety state | Expected mission result | Result |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `NOMINAL-001` | none | — | none | NORMAL / HEALTHY | COMPLETE, `max_alt_err ≤ 10.5 m` | PASS |
| `FAULT_INJECTOR-001` | `FC1_UNAVAILABLE` | t = 20.0 s (permanent) | `FC1_HEARTBEAT_TIMEOUT` ≤ 300 ms | SAFE_MODE | ABORTED (response ≤ 200 ms) | PASS |
| `FAULT_INJECTOR-003` | `INVALID_SENSOR_DATA` | t = 20.0 s, duration 10.0 s | `SENSOR_VALIDATION_FAILED` ≤ 500 ms | DEGRADED → COMPENSATED | continues (not aborted) | PASS |

> The per-scenario detail (objective, configuration, expected behaviour and
> verifications for every launchable scenario on SIL and HIL) lives in the
> [scenario reference](scenarios.md); this table is a quick SIL-engine summary.

A `--all` sweep also runs the **observability suite** (`SilObservability*`:
event ordering, heartbeat/dropped/comms/fault-metadata/logging) and the
**telemetry suite** (`SilObservabilityTelemetry*`: sampling, fault-path, hold),
then emits `docs/validation/data/` artifacts (`sil.md`, `sil.json`, `sil.csv`,
trace `*.jsonl`, telemetry/truth `*.csv`).

In addition to the SIL engine suite, the SIL runner drives the
**physics/autonomous catalog** (`Tests/Scenarios`): `NOMINAL-001` (autonomous
altitude hold, z: 0 → 10 m), `NOMINAL-002..007` (open-loop physics), and
`NOMINAL-008..011` (cascaded X/Y, full mission, 0 → 100 m hold). The full catalog
(`NOMINAL-001..017`) is documented in the [scenario reference](scenarios.md).

## 4. Telemetry

The sensor stream (`sim::sil::SensorTelemetry`) — the **only** data the
controller consumes — exposes:

```text
x  y  z                     position (m)
vx vy vz                   velocity (m/s)
pitch  roll                attitude (rad)
actual_rpm                 effective rotor speed
actual_left_servo          effective servo angles
actual_right_servo
```

A parallel **ground-truth** stream (`TrueStateSample`) records the physics
state (`AircraftState`) at the exact sensor-read instant, so the
sensor-observed path and the simulation truth can be compared per step. The
human-readable mission table is printed at the configured period (default 1 s):

```text
t(s)   x     y     z     vx    vy    vz    pitch  roll  rpm   [mission] [safety]
```

**Distinctions:**

* **Telemetry** = sensor-observed data the controller actually sees (possibly
  corrupted/scaled by the fault injector).
* **Event logs** = structured lifecycle/fault/supervision/safety/recovery
  events (`SilEvent`), independent of telemetry samples.
* **Ground truth** = the physics `AircraftState`, never fed to the controller.

## 5. Limitations — what SIL does *not* prove

* No actual MCU timing, scheduling or RTOS behaviour (single-threaded CPU max).
* No physical peripherals, no real sensors, no real actuators.
* No electrical/power behaviour; no real communication medium (in-process bus).
* Simplified aircraft dynamics (control-software development model).
* Only one sensor channel (altitude/barometer) and one actuator (main rotor)
  are injectable.
* `COMMUNICATION_DEGRADED` has no named deterministic scenario; `CONTROL_DEADLINE_MISSED`
  and `INVALID_NUMERICAL_STATE` are not injectable. See the [FMECA](../fmeca/fmeca.md).
