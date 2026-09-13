# Hardware-in-the-Loop (HIL)

> This documents the project's **actual** HIL architecture — an in-process host
> FC emulator over a loopback byte channel, **not** a physical-MCU setup.
> Related: [scenario reference](scenarios.md) · [test matrix](test_matrix.md) · [architecture overview](../architecture/overview.md).

## 1. Purpose

`HIL_RUNNER` runs the same `flight_sim_core` engine as SIL, but **wall-clock
paced** (1 s simulation time ≈ 1 s wall-clock) and over a **byte-framed
transport**, to exercise the closed loop under real-time scheduling and a
serialised protocol. It validates the loop's *timing* and the *protocol/data
path*, reusing the exact SIL safety/health detection core.

```text
PC (single process)
 ├── Aircraft simulator (Aircraft)
 ├── fault injection (host-side injectors → reused env)
 ├── telemetry / event logging (dual sensor+truth streams)
 └── communication ── loopback byte transport ───► FC target (in-process)
                          │                            │
                          │  SensorPacket (CRC16)       ├── FC1 control + mission
                          └◄ ActuatorPacket (CRC16) ───┤
                                                       └── FC2 (Health/Safety, reused)
```

## 2. Current HIL target (read this)

* **The current target is the in-process host FC emulator** (`HilFcTarget-Host`),
  reached over the `loopback` interface (an in-process byte channel that wraps
  the real `HIL-Proto` codec/parser with CRC16 framing). The run header prints
  `FC target : in-process host FC emulator core (STM32 target is FUTURE)`.
* This does **not** constitute physical-MCU validation. There is no STM32, no
  RTOS, no real peripherals and no serial UART in the current code.
* **What is already ready for the STM32**: the HAL abstractions
  (`SensorInput`, `ActuatorOutput`, `Clock`, byte `Transport`), the
  `HIL-Proto` wire protocol (frame layout, codec, CRC16 parser), the
  `Transport` byte-channel interface, the timing/deadline instrumentation
  (`HilStepTiming`, `HilTimingStats`) and the real-time scheduler
  (`HilRunner::execute`, absolute-deadline pacing). Swapping the
  `LoopbackTransport` for a real serial transport and the host emulator for a
  real FC firmware build is the remaining work.

## 3. Real-time loop

| Parameter | Value | Source |
| :--- | :--- | :--- |
| Control period | 0.01 s | `HilConfig::dt_s` |
| Control frequency | 100 Hz | `1 / dt_s` |
| Mission duration | 30.0 s | `HilConfig::duration_s` |
| Control iterations | 3000 | `hil_step_count` = `duration / dt` (asserted by self-test) |
| Heartbeat timeout | 0.10 s | `HilConfig::heartbeat_timeout_s` |
| Fault injection time | t = 5.0 s | `HilScenarios::hil_fault_for` |
| Real-time pacing | always on | monotonic `std::chrono::steady_clock` |
| Deadline policy | Warn (default) | `HilConfig::deadline_policy` |

The loop schedules each step to an **absolute wall-clock deadline**
(`start_wall + step×period`); overruns are *carried*, not accumulated as drift
(negative relative sleeps). The deadline-miss policy (`Warn`/`Fail`/`Abort`)
controls whether a miss only records, forces a FAIL verdict, or stops the loop.

Terminology used by the runner:

```text
simulation time    advances by dt each step, independent of wall clock
wall-clock time    steady_clock, used only for pacing and timing stats
deadline           scheduled_us + period_us
execution duration step_completion_us − actual_start_us   (step_execution_us)
round trip         actuator_receive_us − sensor_send_us   (round_trip_us)
deadline margin    (scheduled_us + period_us) − step_completion_us
lateness           −deadline_margin
deadline miss      step_completion_us > scheduled_us + period_us
```

## 4. Timing metrics

The `HilRunOutput` summary reports these (`HilTimingStats` + `HilCommStats`):

| Metric | Field | Reported? |
| :--- | :--- | :--- |
| Configured period | `dt_s` (header) | YES |
| Number of iterations | `timing.steps_executed` | YES |
| Expected duration | `duration_s` (header) | YES |
| Worst execution time | `timing.max_step_us` | YES |
| Average execution time | `timing.mean_step_us` | YES |
| Min execution time | `timing.min_step_us` | YES |
| Worst round trip | `timing.max_round_trip_us` | YES |
| Max lateness | `timing.max_lateness_us` | YES |
| Deadline misses | `timing.deadline_misses` | YES |
| Messages sent / received / dropped | `comms.messages_sent/received/dropped` | YES |
| Sequence errors / stale packets | `comms.sequence_errors / stale_packets` | YES |
| Timeouts | `comms.timeouts` | YES |
| Communication latency min/mean/max | `comms.latency_min/mean/max_us` | YES |
| Elapsed wall-clock time | — | **NOT REPORTED** (sim time = 30.0 s; derivable as steps×dt but not printed) |
| Timing jitter (stddev of step time) | — | **NOT MEASURED** as a dedicated metric; variation is bounded by `min/max_step_us` and `max_lateness_us` |

## 5. HIL result (representative)

> The excerpt below is **representative/expected output** of `HIL_RUNNER`,
> derived from the deterministic engine, the 100 Hz / 30 s configuration and the
> reused SIL-baseline safety chain. It was **not captured live in this report's
> environment** (no C++23 toolchain; see the execution note in
> [test_matrix](test_matrix.md)). Run `HIL_RUNNER --scenario NOMINAL-001` on the
> configured toolchain to capture the real artifact.

`NOMINAL-001` — nominal station keeping:

```text
============================================
NOMINAL-001 : nominal station keeping
============================================
Host-target closed-loop validation (NO physical STM32 present).

Configuration
  Duration              : 30.0 s
  Control period        : 0.010 s
  Control frequency     : 100 Hz
  Random seed           : 42
  Real-time pacing      : YES (WARN)
  FC target             : in-process host FC emulator core (STM32 target is FUTURE)

[HIL] t = 0.00 s -> HIL_RUN_START
[MISSION] t = 0.77 s -> CLIMB
[MISSION] t = 2.89 s -> STATION_KEEPING
[MISSION] t = 7.91 s -> MISSION_COMPLETE
... station holding to 30.0 s ...
[HIL] t = 30.00 s -> HIL_RUN_END

Timing
  Steps executed        : 3000
  Deadline misses       : 0
  Max step duration     : ~ tens of us
  Mean step duration    : ~ tens of us
  Max round trip        : ~ tens of us
  Max lateness          : -1 us   (within budget)

Communication statistics
  Messages sent         : 3000
  Messages received     : 3000
  Messages dropped      : 0
  Sequence errors       : 0
  Stale packets         : 0
  Timeouts              : 0

Mission result
  Mission success       : YES
  Final mission state   : COMPLETE
  Final safety mode     : NORMAL
  Final health          : HEALTHY
  Fault detected        : NO
  Test verdict          : PASS
  > nominal mission completed with no abnormal behavior
```

## 6. Did the closed loop execute in real time without missing deadlines?

For the **host emulator**, yes by design: the per-step loop work is on the
order of tens of microseconds, far below the 10 ms budget, so
`deadline_misses == 0` is the deterministic expectation. The HIL **self-test**
(`HIL_RUNNER --selftest`, `test_nominal_run`) asserts exactly:
`steps_executed == 3000`, `deadline_misses == 0`, mission `COMPLETE` and verdict
`PASS`; `test_realtime` asserts wall-clock pacing is active; `test_timing`
asserts the deadline-miss detector and `HilTimingStats` counting. This
conclusion holds for the host-emulator target; it does **not** transfer to the
physical STM32 (not yet present).

## 7. Fault injection in HIL

Faults are injected host-side into the simulated environment through the same
`FaultInjector` set and mirrored into the reused `CommsBus`; the safety/health
core then detects them naturally (same chain as SIL). HIL exposes
the two shared fault scenarios (`FC1_UNAVAILABLE` and `INVALID_SENSOR_DATA`,
activated at t = 5 s, with a 20 s window for the sensor fault). They share
the deterministic outcome of their SIL counterparts (see
[`demonstrations/mission_abort.md`](../demonstrations/mission_abort.md)).

> The per-scenario detail (objective, configuration, expected behaviour and
> verifications for every launchable scenario on SIL and HIL, including the
> nominal profiles `NOMINAL-001` and `NOMINAL-012..017` available in HIL) lives
> in the [scenario reference](scenarios.md).

## 8. Reproduce

```text
HIL_RUNNER --list                       # list scenarios
HIL_RUNNER --scenario NOMINAL-001       # nominal, 30 s, real-time paced
HIL_RUNNER --scenario FAULT_INJECTOR-001   # FC1 failure
HIL_RUNNER --selftest                   # deterministic HIL validation suite
```

## 9. Limitations

* **No physical STM32** — host emulator only; no RTOS, real peripherals,
  interrupt latency or real serial medium are validated.
* **In-process loopback transport** — no real UART/CAN; CRC16 framing and the
  parser are exercised, but not over a lossy physical medium.
* **No jitter statistic**; no printed elapsed wall-clock time.
* Monte Carlo is not run against HIL (it remains a SIL technique).
