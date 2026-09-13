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
- --scenario <id>   Run one shared scenario (NOMINAL-001, FAULT_INJECTOR-001 or FAULT_INJECTOR-003).
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

The full launchable scenario catalog (nominal `NOMINAL-001..017` and fault
injection `FAULT_INJECTOR-001` and `FAULT_INJECTOR-003`) is documented in the
[scenario reference](validation/scenarios.md), which details each scenario's
objective, configuration, expected behaviour and verifications for both the
SIL and HIL runners.

The `SIL_RUNNER` drives two catalogs:

- the **SIL engine suite** (`Tests/Sil/SilScenarios*`): `NOMINAL-001` and
  `FAULT_INJECTOR-001` and `FAULT_INJECTOR-003`;
- the **physics/autonomous catalog** (`Tests/Scenarios`): `NOMINAL-001..017`
  (open-loop physics `002..007`, autonomous cascade/mission `008..011`,
  low takeoff profiles `012..016`, stratosphere `017`).

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

> The per-scenario detail (objective, configuration, expected behaviour and
> verifications for every launchable scenario on SIL and HIL) lives in the
> [scenario reference](validation/scenarios.md). The summary below is kept as a
> quick orientation; refer to the reference for the authoritative descriptions.

All scenarios share the same initial mission: **an autonomous climb toward a target
altitude of 10 meters**, over a total simulated duration of 90 seconds.

The following scenarios are executed:

- **NOMINAL-001 [SIL] — Nominal flight with no fault**: full mission with no fault; must COMPLETE with `NORMAL` safety, no fault detected, altitude error <= 10.5 m.
- **FAULT_INJECTOR-001 [SIL] — FC1 failure**: heartbeat timeout < 300 ms, `SAFE_MODE` (response <= 200 ms), mission aborted.
- **FAULT_INJECTOR-003 [SIL] — Sensor fault**: altitude corruption invalidated < 500 ms, `DEGRADED` -> `COMPENSATED`, mission continues.

> Note: `COMMUNICATION_DEGRADED` (packet loss) is handled at the communication
> layer (`CommsBus`) and has no named deterministic scenario;
> `CONTROL_DEADLINE_MISSED` and `INVALID_NUMERICAL_STATE` have no injection path.
> See the [FMECA](fmeca/fmeca.md).

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
