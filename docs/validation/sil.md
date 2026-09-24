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
* **Telemetry** is sampled at the configured rate (default 20 Hz; dual: sensor-observed + physics
  ground-truth); human-readable table at the configured period.
* **Events** are recorded to a structured `SilTrace` (lifecycle, fault,
  supervision, safety, recovery). Logging is purely observational: it never
  alters the run.

Per-step pipeline (constant step configured by `dt_s`, default `0.01 s`; duration comes from the scenario configuration):

```text
apply_injectors → update_fc1 → update_monitoring → apply_actuators → update_metrics
```

## 3. Exécution et scénarios

Après [compilation](../build/build_targets.md), depuis la racine :

```sh
./artifacts/linux/sil_runner --help
./artifacts/linux/sil_runner --scenario NOMINAL-001
./artifacts/linux/sil_runner --scenario FAULT_INJECTOR-003
./artifacts/linux/sil_runner --all
```

Sans argument, le runner affiche l'aide. `-v` active le détail des observations.
Les réglages hôtes sont lus au démarrage depuis `config/sil.conf`,
`config/simulation.conf` et les 11 profils de `config/scenarios/`. Les options
`--config`, `--simulation-config` et `--scenarios-dir` permettent de changer
ces chemins. Le fichier SIL règle notamment le pas de calcul, les gains du
contrôleur émulé, les seuils de supervision, la télémétrie et le visualiseur ;
`viewer_enabled = false` désactive son replay après un scénario.

Le profil partagé définit la cible et la durée, puis ses overrides explicites
s'appliquent aux réglages du runner. `--telemetry-period` reste prioritaire sur
la période du profil. Les missions autonomes utilisent également le pas et le
contrôleur configurés. Les tests d'observabilité conservent leurs fixtures
indépendantes. Le [guide de configuration](../../config/README.md) décrit le
format, les validations et la frontière avec les firmwares.

Les identités et fenêtres d'injection sont centralisées dans le
[catalogue](scenarios.md). La suite partagée comprend trois scénarios ; `--all`
ajoute les scénarios physiques/autonomes et les suites d'observabilité et de
télémétrie. Attention à la durée du profil stratosphérique du catalogue.

Les résultats de suite sont exportés sous `docs/validation/data/` : résumé
`sil.md`, données `sil.json` / `sil.csv`, traces JSONL et télémétries CSV.
La configuration résolue est écrite avant l'exécution dans
`docs/validation/data/sil_configuration.txt` ; `--config-output <chemin>` permet
de conserver un snapshot distinct par essai, incluant les overrides CLI.
Ces sorties sont ignorées par Git. Appliquer les [conventions de preuve](evidence.md).
Les codes de sortie sont 0 pour succès/aide, 1 pour échec et 2 pour argument ou configuration invalide.

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
