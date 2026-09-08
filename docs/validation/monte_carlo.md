# Monte Carlo Validation (SIL)

> Statistical robustness exploration over the SIL engine. This documents the
> **implemented** campaign. Related: [test matrix](test_matrix.md).

## 1. Objective

Monte Carlo answers: *in the presence of dispersed aircraft, environment and
sensor parameters, does the control law still meet its quality criteria?* It
**complements** deterministic tests — it explores sensitivity and blind spots,
it does **not** replace the strict pass/fail SIL scenarios.

```text
Deterministic SIL scenarios (regression, strict assertions)
                 ⇄  complementary
Monte Carlo dispersion campaign (sensitivity / robustness)
```

## 2. What is actually implemented

There are two Monte Carlo subsystems; **only one is runnable**:

* `SIL_MONTE_CARLO` (runnable) — `sim::monte_carlo` runs a **physics-dispersion**
  campaign over an autonomous scenario from the `ScenarioCatalog`
  (default `NOMINAL-001` = autonomous altitude hold; also `008`, `009`, `010`).
  It perturbs a `PhysicsDispersion` each run, executes the scenario through the
  shared `TestHarness`, and records **control-quality** metrics. It does **not**
  inject faults.
* `sim::sil::validation::MonteCarloRunner` (library, **not wired**) — a
  fault-window Monte Carlo over the SIL engine (`sample_fault_window`,
  `sample_fault_parameters`, `sample_environment`, `sample_initial_conditions`)
  that classifies `SimulationResult`s. It is compiled into the
  `SIL_MONTE_CARLO` target but **not imported by any executable or test**, so
  it has no CLI and no captured results today.

## 3. Variables randomized (`PhysicsDispersion`)

```
Aircraft      mass_variation, cog_offset_{x,y,z}, actuator_gain_dispersion, actuator_lag_dispersion
Environment   wind_speed_mean, wind_heading_rad, turbulence_intensity, atmospheric_density_offset, atmospheric_pressure_offset
Sensors       imu_accel_noise_std, imu_gyro_noise_std, barometer_bias, barometer_drift, gps_latency_jitter
```

Distributions: `sim::DispersionGenerator` uses `std::mt19937_64` with **normal
distributions** for most parameters and **uniform distributions** for bounded
parameters. Every parameter is derived deterministically from the master seed
and the run index.

## 4. Runs and reproducibility

| Item | Value | Source |
| :--- | :--- | :--- |
| Master seed | 42 (default) | `CliOptions::seed` |
| Run count | 50 (default) | `CliOptions::runs` |
| Per-run seed | `generate_run_seed(master_seed, run_index)` | `DispersionGenerator` |
| Scenario | `NOMINAL-001` (default; also `008/009/010`) | `CliOptions::scenario` |
| Export | per-run CSV/JSON (seeds + dispersion + metrics) | `--output-csv` / `--output-json` |

Same master seed → identical dispersion sequence and identical run seeds →
fully reproducible campaign. The CSV/JSON export associates every drawn input
with its measured output, so failures are traceable to a specific dispersion.

## 5. Success / failure and collected statistics

* **Pass/fail per run**: `TestHarness::passed()` (the autonomous scenario's own
  acceptance checks). Failed runs print their drawn inputs for diagnosis.
* **Per-run metrics** (`RunMetrics`): `overshoot`, `settling_time`,
  `steady_state_error`, `max_acceleration`.
* **Campaign statistics** (`print_summary`): total/passed/failed run counts,
  pass rate, and for each metric the **mean, std-dev, min, max** and
  **3-sigma bounds**, plus the list of failed run indices.

## 6. Representative results

The CSV report header (cols: run id, master/run seed, status, all 15 dispersion
inputs, 4 metrics) is real:

```text
run_id;master_seed;run_seed;status;mass_variation;cog_offset_x;...;overshoot;settling_time;steady_state_error;max_acceleration
```

> No campaign was captured in this report's environment (no C++23 toolchain; see
> the execution note in [test_matrix](test_matrix.md)). The earlier version of
> this document contained an *illustrative* 10 000-run example (97.32 % →
> 99.41 % pass rate) — those numbers were **not measured** from the project and
> are not reproduced here. Run `SIL_MONTE_CARLO --runs N --output-csv out.csv`
> on the configured toolchain to capture real statistics.

## 7. Reproduce

```text
SIL_MONTE_CARLO                          # default: seed 42, 50 runs, NOMINAL-001
SIL_MONTE_CARLO --runs 200 --seed 7 -v   # verbose per-run inputs
SIL_MONTE_CARLO --scenario NOMINAL-010 --runs 100 --output-csv mc.csv --output-json mc.json
```

## 8. Limitations

* The runnable campaign is **physics/control-quality only** — it does not
  exercise the safety/fault chain (that is the deterministic SIL scenarios' job).
* The fault-window statistical campaign exists only as library code and is not
  exposed through a CLI.
* Dispersion ranges are engineering placeholders, not calibrated to field data.
* Monte Carlo stays a SIL technique; running large campaigns against HIL is
  out of scope.
