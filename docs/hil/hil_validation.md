# HIL Validation

## 1. Purpose

This document defines the validation strategy for the Hardware-in-the-Loop (HIL) stage of the project.

The objective is to verify that the Flight Controller running on real embedded hardware behaves correctly when connected to the software-based aircraft simulator.

The HIL environment is intended to validate the transition from Software-in-the-Loop (SIL) to real embedded execution while keeping the aircraft, environment, sensors, and actuators simulated.

The HIL system is **not** intended to validate a real aircraft or real flight performance.

---

## 2. Scope

The initial HIL validation covers:

- Flight Controller execution on one real microcontroller;
- RTOS task execution and scheduling;
- communication between the PC simulator and the embedded Flight Controller;
- exchange of simulated sensor data and actuator commands;
- mission execution using the existing aircraft simulator;
- timing and deadline behavior;
- basic fault handling through the existing SIL/HIL fault-injection mechanisms;
- comparison between the established SIL baseline and HIL behavior.

The following are explicitly out of scope for the first HIL stage:

- real flight tests;
- real propulsion;
- real aerodynamic measurements;
- real IMU validation;
- complete replication of the X721 hardware architecture;
- four physical Flight Controllers;
- optical/fiber communication;
- production safety certification.

---

## 3. HIL Architecture

The first HIL configuration uses one real Flight Controller running on a microcontroller and a PC running the aircraft simulator.

```text
                         PC
        +--------------------------------+
        |                                |
        |      Aircraft Simulator        |
        |                                |
        |  Physics / Aerodynamics        |
        |  Environment / Wind            |
        |  Sensor Models                 |
        |  Actuator Models               |
        |                                |
        +---------------+----------------+
                        |
                 SensorPacket
                        |
                 Transport Link
                        |
                        v
        +--------------------------------+
        |          STM32 / MCU           |
        |                                |
        |             RTOS               |
        |                                |
        |  SensorTask                    |
        |  ControlTask                   |
        |  MissionTask                   |
        |  HealthTask                    |
        |  CommunicationTask             |
        |  ActuatorTask                  |
        |                                |
        |       Flight Controller        |
        |                                |
        +---------------+----------------+
                        |
                 ActuatorPacket
                        |
                 Transport Link
                        |
                        v
                       PC
                        |
                        v
                Aircraft Simulator
```

The PC remains the owner of the simulation time and the simulated physical environment.

The embedded target is responsible for running the Flight Controller software under the RTOS.

---

## 4. Validation Principles

HIL validation follows these principles:

### 4.1 Reuse the SIL baseline

HIL scenarios should be derived from scenarios that already pass in SIL whenever possible.

This provides a known software baseline before introducing hardware-specific behavior.

### 4.2 Keep the control logic unchanged

The Flight Controller application logic should not be rewritten specifically for HIL.

The goal is to replace simulated/platform-specific dependencies through hardware abstraction and transport implementations while keeping the control behavior equivalent.

### 4.3 Separate simulator truth from sensor data

The PC simulator has access to the true aircraft state.

The Flight Controller must receive only the simulated sensor measurements exposed through the sensor interface.

The Flight Controller must not directly access the simulator's true state.

### 4.4 Make timing observable

The HIL system must make it possible to measure:

- control-loop period;
- execution time;
- deadline misses;
- communication latency;
- sensor-to-control latency;
- control-to-actuator latency.

### 4.5 Preserve reproducibility

A HIL scenario must identify:

- scenario ID;
- configuration;
- random seed where applicable;
- software version;
- firmware version;
- simulation configuration;
- relevant hardware configuration.

---

## 5. SIL Baseline

Before running HIL validation, the same scenario should be executed in SIL and recorded as the reference result.

At minimum, the baseline should contain:

- mission result;
- mission state transitions;
- final aircraft state;
- maximum position error;
- maximum altitude error;
- maximum pitch and roll;
- communication metrics where applicable;
- detected faults;
- detection latency;
- safety response latency.

Example:

```text
Scenario: NOMINAL-001
Target position: (0, 0)
Target altitude: 100 m
Mission duration: TBD

Mission result: PASS
Final mission state: COMPLETE
Max position error: TBD
Max altitude error: TBD
```

The exact numerical tolerances must come from the scenario definition and not be invented by the HIL runner.

---

## 6. HIL Test Categories

The HIL campaign is divided into several categories.

### NOMINAL-001 — Nominal startup

Objective:

Verify that the embedded Flight Controller starts correctly and establishes communication with the simulator.

Expected behavior:

- MCU boots successfully;
- RTOS starts successfully;
- required tasks start;
- communication link becomes operational;
- Flight Controller enters its expected initial state;
- no safety fault is raised unexpectedly.

---

### PROTO — Sensor packet exchange

Objective:

Verify that simulated sensor measurements generated by the PC reach the Flight Controller correctly.

Expected behavior:

- sensor packets are transmitted;
- packets are decoded correctly;
- timestamps and sequence numbers are valid;
- the Flight Controller uses the received sensor values;
- invalid packets are rejected according to the communication protocol.

---

### PROTO — Actuator command exchange

Objective:

Verify that the embedded Flight Controller produces actuator commands that are correctly received by the simulator.

Expected behavior:

- actuator commands are transmitted;
- commands are decoded correctly;
- simulated actuators apply the commands;
- the aircraft simulation responds accordingly.

---

### NOMINAL-001 — Nominal mission

Objective:

Run an already validated SIL mission on the embedded Flight Controller.

Example mission:

```text
TAKEOFF
   -> CLIMB
   -> STATION_KEEPING
   -> COMPLETE
```

Expected behavior:

- mission state transitions occur correctly;
- aircraft reaches the target altitude;
- aircraft remains within the operational zone;
- control remains stable;
- no unexpected supervision or timing faults occur (heartbeat/link supervision and real-time deadline).

The HIL result should be compared with the SIL baseline.

---

### TIMING — RTOS timing validation

Objective:

Verify execution timing of the embedded Flight Controller tasks.

At minimum measure:

- task period;
- actual execution time;
- worst-case observed execution time;
- deadline misses;
- scheduling jitter where measurable.

For example:

```text
ControlTask
Period:              5 ms
Expected frequency:  200 Hz
Worst execution:     TBD
Deadline misses:     0
```

The values are examples only; final requirements must be defined from the actual system configuration.

---

### TIMING — Communication latency

Objective:

Measure the latency introduced by the HIL transport.

Measure separately where possible:

```text
PC -> MCU sensor latency
MCU -> PC actuator latency
Round-trip latency
```

The measurements must use simulation timestamps and/or synchronized timestamps defined by the HIL protocol.

---

### FAULT_INJECTOR-003 — Sensor fault

Objective:

Verify that a simulated sensor failure is propagated through the actual HIL data path.

Example:

```text
PC sensor model
      |
      | sensor fault injected
      v
SensorPacket
      |
      v
STM32 Flight Controller
      |
      v
Health Monitoring
```

Expected behavior:

- the fault is injected by the simulator/fault injector;
- the resulting sensor measurement is actually received by the embedded Flight Controller;
- the Flight Controller detects or propagates the fault according to the defined safety logic;
- the appropriate safety state is reached.

The fault must not be injected by directly modifying the expected HIL result.

---

### FAULT_INJECTOR-002 — Communication loss

Objective:

Verify behavior when the HIL transport stops delivering messages.

Example:

```text
PC  ------X------> MCU
```

Expected behavior depends on the type of communication being interrupted, but should be consistent with the communication and safety requirements.

The test should record:

- last successfully received sequence number;
- time of communication loss;
- timeout detection time;
- safety response;
- recovery behavior if recovery is supported.

---

### SAFETY — Watchdog / task stall

Objective:

Verify that a critical task stall is detected by the embedded watchdog mechanism.

Example:

```text
ControlTask
    |
    X  no progress
    |
Watchdog
    |
    v
Fault / recovery
```

Expected behavior:

- the task stops making progress;
- watchdog conditions are no longer satisfied;
- watchdog timeout is detected;
- defined recovery/safety behavior occurs.

---

## 7. HIL Telemetry and Event Logging

HIL must preserve the observability architecture used by SIL.

The validation data should contain at least three categories of information:

### 7.1 Aircraft telemetry

Periodic telemetry should include, where available:

- simulation timestamp;
- X;
- Y;
- altitude;
- X velocity;
- Y velocity;
- vertical velocity;
- pitch;
- roll;
- commanded wing RPM;
- actual wing RPM;
- left servo command;
- right servo command;
- target position;
- target altitude;
- mission state;
- safety state.

Telemetry should represent the real data path used by the Flight Controller where the measurement is intended to represent controller-observed state.

### 7.2 Event trace

Discrete events should remain separately observable:

- mission state transition;
- safety state transition;
- fault injection;
- fault detection;
- watchdog timeout;
- communication timeout;
- recovery.

### 7.3 Ground truth

The simulator may additionally record ground-truth aircraft state for validation.

Ground truth must remain explicitly separated from sensor telemetry.

Example:

```text
TRUE STATE
altitude = 100.00 m

SENSOR DATA
altitude = 99.72 m

FC DECISION
RPM command = 812
```

This separation is essential when validating sensor faults, noise, latency, and estimator behavior.

---

## 8. Timing and Synchronization

The PC is the master of simulation time in the initial HIL architecture.

The embedded Flight Controller must process data using the timestamps and sequencing rules defined by the HIL protocol.

The HIL implementation must make it possible to distinguish:

```text
simulation time
transport latency
embedded processing time
actuator application time
```

Avoid using wall-clock time as the primary source of simulation truth.

---

## 9. SIL vs HIL Comparison

For each scenario executed in both environments, compare the relevant results.

### Mission-level comparison

- mission completed/aborted;
- mission state sequence;
- time to target altitude;
- station-keeping performance;
- final position;
- final altitude.

### Control-level comparison

- pitch;
- roll;
- commanded RPM;
- servo commands;
- control-loop timing.

### Safety comparison

- faults detected;
- detection latency;
- safety state transitions;
- watchdog behavior;
- recovery behavior.

### Communication comparison

- packet counts;
- latency;
- packet loss;
- sequence errors;
- timeout events.

The comparison should focus on meaningful tolerances rather than exact floating-point equality.

---

## 10. Acceptance Criteria

A HIL scenario passes when all requirements defined by that scenario are satisfied.

A generic nominal HIL run should, at minimum, demonstrate:

- successful embedded startup;
- stable communication with the simulator;
- correct sensor packet processing;
- correct actuator command generation;
- successful mission execution;
- no unexpected deadline misses;
- no unexpected watchdog reset;
- telemetry consistent with the SIL baseline within defined tolerances.

A fault-injection HIL test passes when the system responds according to the defined fault-handling requirements, even if the mission itself is intentionally unsuccessful.

Therefore:

```text
Mission success != Test verdict
```

For example:

```text
Mission success: NO
Test verdict:    PASS
```

is valid when a fault is intentionally injected and the safety system responds exactly as required.

---

## 11. HIL Result Format

Each HIL run should produce a structured result containing at least:

```text
scenario_id
software_version
firmware_version
hardware_configuration
random_seed
mission_result
final_mission_state
final_safety_state
final_position
final_altitude
max_position_error
max_altitude_error
max_pitch
max_roll
fault_information
detection_latency
response_latency
recovery_information
communication_metrics
timing_metrics
watchdog_status
verdict
```

The result should reference the detailed telemetry/event files generated during the run.

---

## 12. Regression Strategy

HIL validation should not replace SIL regression testing.

The recommended order for a changed Flight Controller version is:

```text
Code change
   |
   v
SIL regression
   |
   +---- failure -> fix before HIL
   |
   v
HIL nominal tests
   |
   v
HIL fault tests
   |
   v
Compare against baseline
```

A change should not be promoted to HIL simply because the source code builds successfully.

---

## 13. Reproducibility

Every HIL run should record enough information to reproduce or diagnose the result.

At minimum:

```text
Scenario ID
Simulation configuration
Random seed
Software commit
Firmware build
Hardware target
HIL protocol version
RTOS configuration
```

For failing runs, preserve:

- telemetry;
- event trace;
- structured result;
- configuration;
- seed;
- firmware/software identifiers.

---

## 14. Fault Injection in HIL

The same fault-injection philosophy used by SIL should be retained in HIL.

The PC should inject faults into the simulated environment or communication path rather than directly forcing a safety-state result.

Example:

```text
Fault Injector
      |
      | ALTITUDE_SENSOR_COMMUNICATION_LOSS
      v
Sensor / Transport
      |
      v
STM32 Flight Controller
      |
      v
Detection
      |
      v
Safety Response
```

This allows HIL to validate the complete chain instead of testing only the safety manager in isolation.

---

## 15. Recommended Initial HIL Campaign

The first campaign should be deliberately small.

### NOMINAL-001
Nominal startup.

### PROTO
Sensor packet exchange.

### PROTO
Actuator packet exchange.

### NOMINAL-001
Nominal station-keeping mission.

### TIMING
Timing/deadline measurement.

### TIMING
Communication latency measurement.

### FAULT_INJECTOR-003
Sensor fault injection.

### FAULT_INJECTOR-002
Communication loss.

### SAFETY
Watchdog/task-stall test.

Only after these tests are stable should the campaign be expanded.

---

## 16. Future Extensions

The following are deliberately not required for the first HIL implementation but may be added later:

- second physical Flight Controller;
- physical FC1 <-> FC2 communication;
- CAN transport;
- real sensor interfaces;
- real actuator interfaces;
- additional hardware targets;
- automated HIL regression campaigns;
- Monte Carlo directly against HIL for a limited set of scenarios.

Monte Carlo should remain primarily a SIL validation technique because running large campaigns on real hardware is considerably more expensive and slower.

---

## 17. Final Validation Goal

The HIL stage is successful when the project can demonstrate the following chain:

```text
Digital Twin / Aircraft Simulator
            |
            v
      Simulated Sensors
            |
            v
       Real RTOS / MCU
            |
            v
      Flight Controller
            |
            v
     Real-time commands
            |
            v
      Simulated Aircraft
```

and can show, using recorded telemetry and event traces, that:

1. the embedded Flight Controller receives the intended sensor information;
2. the control software executes within its timing constraints;
3. actuator commands reach the simulated aircraft correctly;
4. the aircraft follows the intended mission within defined tolerances;
5. faults can be injected through the intended HIL path;
6. faults are detected and handled according to the safety design;
7. HIL behavior is comparable to the established SIL baseline where equivalence is expected.
