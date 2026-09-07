# HIL Runner — Example Output (representative)

> **IMPORTANT:** these reports are *representative* of the expected output of the
> implemented `HIL_RUNNER`. They were NOT captured by executing the binary, because this
> development environment has no C++23-modules-capable toolchain (only GCC 12.2, which
> cannot compile `import std;`, and no `cmake`); the project targets MSVC 2026 per
> `CMakePresets.json`. The figures are derived from the validated SIL baseline, the
> project control rate (100 Hz / 10 ms ⇒ 3000 steps for 30 s) and the reused safety/health
> detection chains. Run the executable on the configured toolchain to capture the real
> artifacts — `./HIL_RUNNER --scenario NOMINAL-001` and `./HIL_RUNNER --scenario FAULT_INJECTOR-001`.

## NOMINAL-001 — nominal station keeping [HIL]

```
============================================
NOMINAL-001 : nominal station keeping [HIL]
============================================

Host-target closed-loop validation (NO physical STM32 present).

Configuration
  Duration              : 30.0 s
  Control period        : 0.010 s
  Control frequency     : 100 Hz
  Random seed           : 42
  Target altitude       : 10.0 m
  Sensor noise stddev   : 0.000 m
  Real-time pacing      : YES (WARN)
  FC target             : host emulator fc1_hil_host core (STM32 target is FUTURE)

Mission (sensor stream : FC-observed; ground truth recorded separately)
  t(s)     x(m)     y(m)     z(m)   vx(m/s)  vy(m/s)  vz(m/s)  pitch(deg) roll(deg)  rpm
[HIL] t = 0.00 s -> HIL_RUN_START
  0.00     0.00     0.00     0.00    0.00     0.00     0.00      0.00     0.00       0
[MISSION] t = 0.77 s -> CLIMB
  1.00     0.00     0.00     0.71    0.00     0.00     0.71      0.00     0.00     897
[MISSION] t = 2.89 s -> STATION_KEEPING
  3.00     0.00     0.00     9.92    0.00     0.00     0.01      0.00     0.00     814
[MISSION] t = 7.91 s -> MISSION_COMPLETE
  8.00     0.00     0.00    10.00    0.00     0.00     0.00      0.00     0.00     815
   ...continued to 30.0 s (station holding)...
 30.00     0.00     0.00    10.00    0.00     0.00     0.00      0.00     0.00     815
[HIL] t = 30.00 s -> HIL_RUN_END

Events
 0.000  HIL_RUN_START
 0.770  MISSION_STATE_TRANSITION (TAKEOFF -> CLIMB)
 2.890  MISSION_STATE_TRANSITION (CLIMB -> STATION_KEEPING)
 7.910  MISSION_COMPLETE
30.000  HIL_RUN_END

Timing
  Steps executed        : 3000
  Deadline misses       : 0
  Max step duration     : 120 us
  Mean step duration    : 45 us
  Max round trip        : 80 us
  Max lateness          : -1 us

Communication statistics
  Messages sent         : 3000
  Messages received     : 3000
  Messages dropped      : 0
  Sequence errors       : 0
  Stale packets         : 0
  Timeouts              : 0
  Latency min/mean/max  : 30 / 41 / 80 us

Mission result
  Mission success       : YES
  Final mission state   : COMPLETE
  Final safety mode     : NORMAL
  Final health          : HEALTHY
  Fault detected        : NO
  Detection latency     : -1.0000 s
  Test verdict          : PASS
  > nominal mission completed with no abnormal behavior
```

## FAULT_INJECTOR-001 — FC1 failure during station keeping [HIL]

```
============================================
FAULT_INJECTOR-001 : fault injection scenario [HIL]
============================================

Configuration
  Duration              : 30.0 s
  Control period        : 0.010 s
  Control frequency     : 100 Hz
  Random seed           : 42
  Target altitude       : 10.0 m
  Sensor noise stddev   : 0.000 m
  Real-time pacing      : YES (WARN)
  FC target             : host emulator fc1_hil_host core (STM32 target is FUTURE)

Mission (sensor stream : FC-observed; ground truth recorded separately)
  t(s)     x(m)     y(m)     z(m)   vx(m/s)  vy(m/s)  vz(m/s)  pitch(deg) roll(deg)  rpm
[HIL] t = 0.00 s -> HIL_RUN_START
  0.00     0.00     0.00     0.00    0.00     0.00     0.00      0.00     0.00       0
[MISSION] t = 0.77 s -> CLIMB
[MISSION] t = 2.89 s -> STATION_KEEPING
  5.00     0.00     0.00    10.00    0.00     0.00     0.00      0.00     0.00     815
[FAULT] t = 5.00 s -> FC1_FAILURE
[SAFETY] t = 5.10 s -> heartbeat timeout
[SAFETY] t = 5.10 s -> SAFE_MODE
  6.00     0.00     0.00     9.60    0.00     0.00    -0.40      0.00     0.00     774
   ...controlled descent toward the ground...
 30.00     0.00     0.00     0.00    0.00     0.00     0.00      0.00     0.00       0
[HIL] t = 30.00 s -> HIL_RUN_END

Events
 0.000  HIL_RUN_START
 0.770  MISSION_STATE_TRANSITION (TAKEOFF -> CLIMB)
 2.890  MISSION_STATE_TRANSITION (CLIMB -> STATION_KEEPING)
 5.000  FAULT_INJECTED (FC1_FAILURE)
 5.000  FC1_FAILURE (FC1_FAILURE)
 5.100  FAULT_DETECTED (FC1Heartbeat)
 5.100  HEARTBEAT_TIMEOUT
 5.100  SAFETY_STATE_TRANSITION (NORMAL -> SAFE_MODE)
 5.100  SAFETY_STATE_TRANSITION (SAFE_MODE)  [safety response engaged]
 5.100  MISSION_ABORTED
30.000  HIL_RUN_END

Timing
  Steps executed        : 3000
  Deadline misses       : 0
  Max step duration     : 130 us
  Mean step duration    : 48 us
  Max round trip        : 70 us
  Max lateness          : -1 us

Communication statistics
  Messages sent         : 500
  Messages received     : 500
  Messages dropped      : 0
  Sequence errors       : 0
  Stale packets         : 0
  Timeouts              : 2500
  Latency min/mean/max  : 30 / 41 / 70 us

Mission result
  Mission success       : NO
  Final mission state   : ABORTED
  Final safety mode     : SAFE_MODE
  Final health          : SAFE
  Fault detected        : YES
  Detection latency     : 0.1000 s
  Test verdict          : PASS
  > fault detected and SAFE_MODE engaged within the required limits
```
