# test_simulation — Physical simulator validation

Physical simulator validation (Heliblade-like). The binary exercises the aircraft
physical model and the closed-loop flight controller against a catalog of deterministic
scenarios and verifies their response.

Executable path: `build/Debug/test_simulation` (run as `test_simulation`).

## Usage

```text
Usage: test_simulation [--target=simulation] [scenario ...]
- With no scenario argument, all scenarios are executed (in catalog order).
- --target=simulation   Execution target (default: simulation, also accepts: sim). The target is paired with each scenario ID in the logs, e.g. [NOM-002_GroundedRest][SIM].
- scenario              One or more functional scenario IDs from the table below.
- -h, --help            Show this help and exit.
- Any unknown argument is rejected with an error and the usage is printed.
```

Examples:

```text
# All scenarios (in catalog order)
test_simulation

# A single scenario (autonomous altitude hold)
test_simulation MC-001_AltitudeHold

# Several scenarios in sequence
test_simulation NOM-002_GroundedRest NOM-004_Descent MC-004_FullMission
```

## Available scenarios

| ID | Description | Function |
|----|-------------|----------|
| `NOM-002_GroundedRest` | Grounded rest (RPM = 0, servos = 0) | `scenarios::rest` |
| `NOM-003_VerticalClimb` | Vertical climb (RPM > hover) | `scenarios::climb` |
| `NOM-004_Descent` | Descent (RPM < hover) | `scenarios::descent` |
| `NOM-005_ForwardTranslation` | Forward translation (hover + pitch > 0) | `scenarios::move_x` |
| `NOM-006_LateralTranslation` | Lateral translation (hover + roll > 0) | `scenarios::move_y` |
| `NOM-007_CombinedTranslation` | Combined translation (RPM > hover, pitch > 0, roll < 0) | `scenarios::combined` |
| `MC-001_AltitudeHold` | Autonomous altitude hold (z: 0 -> 100 m) | `flight_scenarios::autonomous_altitude` |
| `MC-002_PositionXHold` | Autonomous cascaded X axis (x: 20 -> 0) | `flight_scenarios::autonomous_position_x` |
| `MC-003_PositionYHold` | Autonomous cascaded Y axis (y: -15 -> 0) | `flight_scenarios::autonomous_position_y` |
| `MC-004_FullMission` | Autonomous full mission (TAKEOFF to COMPLETE) | `flight_scenarios::autonomous_mission` |

### Physics scenarios (NOM-002 – NOM-007)

Deterministic open-loop scenarios: they verify the response of the physical model to
motor and servo commands.

- **NOM-002_GroundedRest — rest**: aircraft on the ground, no command; it must remain motionless.
- **NOM-003_VerticalClimb — climb**: RPM above the theoretical hover value; vertical climb is expected.
- **NOM-004_Descent — descent**: climb followed by throttle reduction; return to the ground is expected.
- **NOM-005_ForwardTranslation — move_x**: positive mean servo command (+10 degrees) -> pitch > 0.
- **NOM-006_LateralTranslation — move_y**: opposed servos (+12 / -12 degrees) -> pure differential, roll > 0 with no pitch.
- **NOM-007_CombinedTranslation — combined**: positive mean (+5 degrees) and negative differential -> pitch > 0 and roll < 0.

### Autonomous mode-change scenarios (MC-001 – MC-004)

Closed-loop scenarios driving the flight controller.

- **MC-001_AltitudeHold — autonomous_altitude**: autonomous altitude loop, convergence toward z = 100 m with metrics.
- **MC-002_PositionXHold — autonomous_position_x**: X position -> pitch -> servo cascade, return from x = 20 m to x = 0.
- **MC-003_PositionYHold — autonomous_position_y**: Y position -> roll -> servo cascade, return from y = -15 m to y = 0.
- **MC-004_FullMission — autonomous_mission**: full mission, from the TAKEOFF state through to COMPLETE.

## Execution conditions

- Deterministic single-thread loop at 100 Hz (`dt = 0.01 s`).
- A single shared harness accumulates the failure counter across all launched scenarios.
- The theoretical hover RPM is computed from the reference aircraft parameters and printed at startup.

## Exit codes

| Code | Meaning |
|------|----------|
| `0` | All launched scenarios passed (PASS), or help shown via `-h`. |
| `1` | At least one verification failed (FAIL). |
| `2` | Invalid command-line argument. |

## SIL validation (Software-in-the-Loop)

A dedicated executable, `test_sil`, validates the robustness of the system against faults
(Software-in-the-Loop).

The SIL suite is run via `test_sil --target=sil` and the HIL suite via
`test_hil --target=hil`. The scenario IDs are shared (target-agnostic) and the target is
paired with each ID in the logs (e.g. `[NOM-001][SIL]`, `[FINJ-001][HIL]`).

### Implemented SIL scenarios

All scenarios share the same initial mission: **an autonomous climb toward a target
altitude of 10 meters**, over a total simulated duration of 90 seconds.

The following scenarios are executed:

- **NOM-001_StationKeeping [SIL] — Nominal flight with no fault**
  - **Description**: Full mission execution with no fault injection.
  - **Expectations**: The mission must complete with the `COMPLETE` status, the safety mode must remain `NORMAL`, no fault must be detected, and the altitude error must stay controlled (<= 10.5 m).

- **FINJ-001_Fc1Failure [SIL] — Primary flight controller failure (FC1 failure)**
  - **Description**: Abrupt stop of the primary flight controller at t = 30.0 s (during altitude hold).
  - **Expectations**: The fault must be detected via a heartbeat timeout in less than 300 ms. The system must switch to `SAFE_MODE` with a response latency <= 200 ms, and the mission must be aborted.

- **FINJ-002_CommLoss [SIL] — Total communication loss (Communication loss)**
  - **Description**: Communication link cut at t = 30.0 s.
  - **Expectations**: The `COMMUNICATION_LOST` alert must be raised in less than 300 ms. The system must engage a safety reaction (`SAFE_MODE`) and abort the mission.

- **FINJ-003_SensorFault [SIL] — Sensor failure (Sensor fault)**
  - **Description**: Corruption of the altitude sensor data (outlier value) at t = 20.0 s for 10 seconds.
  - **Expectations**: The outlier value must be invalidated in less than 500 ms. The `HealthMonitor` must enter the `DEGRADED` state, the `COMPENSATED` mode must be engaged to maintain flight, and the mission must not be aborted.

- **FINJ-004_ActuatorDegradation [SIL] — Actuator degradation (Actuator degradation)**
  - **Description**: Drop of an actuator efficiency to 60% of its capacity at t = 15.0 s (during the climb phase).
  - **Expectations**: The mismatch between the command and the physical response must be detected. The system must enter the `DEGRADED` state, engage a compensation setpoint, and continue the mission without aborting it.

An additional SIL scenario is also available:

- **MC-FINJ-001_Fc1FailureDuringClimb [SIL] — FC1 failure during the climb mode-change transition**
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
