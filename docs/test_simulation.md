# SIL_RUNNER — Physical simulator validation

Physical simulator validation (Heliblade-like). The binary exercises the aircraft
physical model and the closed-loop flight controller against a catalog of deterministic
scenarios and verifies their response, running the full deterministic SIL suite at
maximum CPU speed.

Executable path: `build/SIL_RUNNER` (run as `SIL_RUNNER`).

## Usage

```text
Usage: SIL_RUNNER [--scenario <id>] [--all] [-v | --verbose]
- With no argument, the usage helper is printed: available options and the full
  pre-configured scenario catalog. Nothing is executed.
- --scenario <id>   Run one scenario (NOMINAL-001..011, FAULT_INJECTOR-001..005).
- --all             Run the full deterministic sweep (every scenario, in order).
- -v, --verbose     Per-step telemetry logging.
- -h, --help        Show this help and exit.
- Any unknown argument is rejected with an error and the usage is printed.
```

Examples:

```text
# Print the helper (options + pre-configured scenario catalog), run nothing
SIL_RUNNER

# SIL engine suite + physics/autonomous catalog (in order)
SIL_RUNNER --all

# A single engine scenario (nominal station keeping)
SIL_RUNNER --scenario NOMINAL-001

# A single fault-injection scenario
SIL_RUNNER --scenario FAULT_INJECTOR-001

# A single autonomous scenario
SIL_RUNNER --scenario NOMINAL-011
```

## Available scenarios

| ID | Description | Function |
|----|-------------|----------|
| `NOMINAL-002` | Grounded rest (RPM = 0, servos = 0) | `scenarios::rest` |
| `NOMINAL-003` | Vertical climb (RPM > hover) | `scenarios::climb` |
| `NOMINAL-004` | Descent (RPM < hover) | `scenarios::descent` |
| `NOMINAL-005` | Forward translation (hover + pitch > 0) | `scenarios::move_x` |
| `NOMINAL-006` | Lateral translation (hover + roll > 0) | `scenarios::move_y` |
| `NOMINAL-007` | Combined translation (RPM > hover, pitch > 0, roll < 0) | `scenarios::combined` |
| `NOMINAL-008` | Autonomous cascaded X axis (x: 20 -> 0) | `flight_scenarios::autonomous_position_x` |
| `NOMINAL-009` | Autonomous cascaded Y axis (y: -15 -> 0) | `flight_scenarios::autonomous_position_y` |
| `NOMINAL-010` | Autonomous full mission (TAKEOFF to COMPLETE) | `flight_scenarios::autonomous_mission` |
| `NOMINAL-011` | Autonomous altitude hold (z: 0 -> 100 m) | `flight_scenarios::autonomous_altitude` |
| `NOMINAL-012` | Low vertical takeoff (30 s, z = 5 m) | `flight_scenarios::low_vertical_takeoff` |
| `NOMINAL-013` | Low forward takeoff (30 s, x = 4 m, z = 6 m) | `flight_scenarios::low_forward_takeoff` |
| `NOMINAL-014` | Low lateral takeoff (30 s, y = -4 m, z = 7 m) | `flight_scenarios::low_lateral_takeoff` |
| `NOMINAL-015` | Low diagonal takeoff (30 s, x = 3 m, y = 3 m, z = 8 m) | `flight_scenarios::low_diagonal_takeoff` |
| `NOMINAL-016` | Low offset takeoff (30 s, x = -3 m, y = 2 m, z = 9 m) | `flight_scenarios::low_offset_takeoff` |
| `NOMINAL-017` | Stratosphere climb (about 9 h, z = 20 km) | `flight_scenarios::stratosphere_climb` |

### Physics scenarios (NOMINAL-002 – NOMINAL-007)

Deterministic open-loop scenarios: they verify the response of the physical model to
motor and servo commands.

- **NOMINAL-002 — rest**: aircraft on the ground, no command; it must remain motionless.
- **NOMINAL-003 — climb**: RPM above the theoretical hover value; vertical climb is expected.
- **NOMINAL-004 — descent**: climb followed by throttle reduction; return to the ground is expected.
- **NOMINAL-005 — move_x**: positive mean servo command (+10 degrees) -> pitch > 0.
- **NOMINAL-006 — move_y**: opposed servos (+12 / -12 degrees) -> pure differential, roll > 0 with no pitch.
- **NOMINAL-007 — combined**: positive mean (+5 degrees) and negative differential -> pitch > 0 and roll < 0.

### Autonomous mission scenarios (NOMINAL-008 – NOMINAL-016)

Closed-loop scenarios driving the flight controller.

- **NOMINAL-008 — autonomous_position_x**: X position -> pitch -> servo cascade, return from x = 20 m to x = 0.
- **NOMINAL-009 — autonomous_position_y**: Y position -> roll -> servo cascade, return from y = -15 m to y = 0.
- **NOMINAL-010 — autonomous_mission**: full mission, from the TAKEOFF state through to COMPLETE.
- **NOMINAL-011 — autonomous_altitude**: autonomous altitude loop, convergence toward z = 100 m with metrics.
- **NOMINAL-012 to NOMINAL-016 — low takeoff profiles**: five 30-second flights from rest at 5 m to 9 m,
  combining vertical hold, forward, lateral, and diagonal translations. They intentionally end airborne.
- **NOMINAL-017 — stratosphere_climb**: 20 km climb at a capped 0.617 m/s, reaching the target after
  approximately nine real hours because HIL always runs at a 1:1 wall-clock rate.

## Execution conditions

- Deterministic single-thread loop at 100 Hz (`dt = 0.01 s`).
- Autonomous takeoff starts with a 3-second rotor spin-up, followed by a 12-second smoothstep transition
  toward the 0.617 m/s Heliblade-derived climb speed.
- A single shared harness accumulates the failure counter across all launched scenarios.
- The theoretical hover RPM is computed from the reference aircraft parameters and printed at startup.

## Exit codes

| Code | Meaning |
|------|----------|
| `0` | All launched scenarios passed (PASS), or help shown via `-h`. |
| `1` | At least one verification failed (FAIL). |
| `2` | Invalid command-line argument. |

## SIL validation (Software-in-the-Loop)

`SIL_RUNNER` validates the robustness of the system against faults
(Software-in-the-Loop) and executes the deterministic physics/autonomous catalog.
Scenario IDs follow the standardised taxonomy (NOMINAL-xxx, FAULT_INJECTOR-xxx) and are
paired with each ID in the
logs (e.g. `[NOMINAL-001][SIL]`, `[FAULT_INJECTOR-001][SIL]`). The HIL suite lives in
`HIL_RUNNER --selftest`.

### Implemented SIL scenarios

All scenarios share the same initial mission: **an autonomous climb toward a target
altitude of 10 meters**, over a total simulated duration of 90 seconds.

The following scenarios are executed:

- **NOMINAL-001 [SIL] — Nominal flight with no fault**
  - **Description**: Full mission execution with no fault injection.
  - **Expectations**: The mission must complete with the `COMPLETE` status, the safety mode must remain `NORMAL`, no fault must be detected, and the altitude error must stay controlled (<= 10.5 m).

- **FAULT_INJECTOR-001 [SIL] — Primary flight controller failure (FC1 failure)**
  - **Description**: Abrupt stop of the primary flight controller at t = 30.0 s (during altitude hold).
  - **Expectations**: The fault must be detected via a heartbeat timeout in less than 300 ms. The system must switch to `SAFE_MODE` with a response latency <= 200 ms, and the mission must be aborted.

- **FAULT_INJECTOR-002 [SIL] — Total communication loss (Communication loss)**
  - **Description**: Communication link cut at t = 30.0 s.
  - **Expectations**: The `COMMUNICATION_TIMEOUT` detection event must be raised in less than 300 ms. The system must engage a safety reaction (`SAFE_MODE`) and abort the mission.

- **FAULT_INJECTOR-003 [SIL] — Sensor failure (Sensor fault)**
  - **Description**: Corruption of the altitude sensor data (outlier value) at t = 20.0 s for 10 seconds.
  - **Expectations**: The outlier value must be invalidated in less than 500 ms. The `HealthMonitor` must enter the `DEGRADED` state, the `COMPENSATED` mode must be engaged to maintain flight, and the mission must not be aborted.

- **FAULT_INJECTOR-004 [SIL] — Actuator degradation (Actuator degradation)**
  - **Description**: Drop of an actuator efficiency to 60% of its capacity at t = 15.0 s (during the climb phase).
  - **Expectations**: The mismatch between the command and the physical response must be detected. The system must enter the `DEGRADED` state, engage a compensation setpoint, and continue the mission without aborting it.

An additional SIL scenario is also available:

- **FAULT_INJECTOR-005 [SIL] — FC1 failure during the climb transition**
  - **Description**: An FC1 failure is injected during the climb mode-change transition, exercising the safety chain across a mode switch rather than during steady station keeping.
  - **Expectations**: The failure must be detected through the heartbeat timeout, `SAFE_MODE` must be engaged within the required latency, and the mission must be aborted.

*(Note: Network packet loss (Packet loss) is handled at the communication layer level and can be tested via similar probabilistic scenarios.)*

### Pass/Fail criteria

Each scenario evaluates a `SimulationResult` object containing the flight metrics. The
success criteria include:
- Correct detection of the injected fault.
- A detection latency that respects the real-time constraints (e.g. <= 300 ms).
- A switch into an appropriate safety mode (`SAFE_MODE`, `COMPENSATED`, etc.).
- Aborting the mission when required.
- Keeping the altitude error within acceptable limits.

### Validation reports

At the end of execution, the results are exported automatically to the
`docs/validation/data/` folder in several formats:

- **`sil.md`**: An automatically generated Markdown file that presents the results in a human-readable form. It contains a formatted summary table and the textual details of each scenario, ready to be read directly on a Git repository or a wiki.
- **`sil.json`**: A JSON file containing all the raw simulation data (scenario configuration, flight metrics, detection times, latencies, verdicts). This format is designed to be read by machines, making it ideal for continuous integration (CI) and automated parsing.
- **`sil.csv`**: A CSV (Comma-Separated Values) file summarizing the results as a table (Test name, Verdict, Detection time, Latency, Final mode). This format is perfect for a quick import into a spreadsheet (Excel, Calc) or for generating analysis charts.

Each scenario section pairs the scenario ID with the `[SIL]` target tag.

These files constitute the proof of automated validation of the system.
