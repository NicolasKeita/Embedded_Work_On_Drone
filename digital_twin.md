# Digital Twin — Real-Time HIL Visualization

## 1. Purpose

This project uses a **real-time Digital Twin / HIL testbed** to validate a distributed flight-control architecture composed of two STM32-based Flight Controllers.

The Digital Twin is **not the 3D viewer itself**. The core twin is the executable digital aircraft model running on the Linux host and interacting in closed loop with the real FC1/FC2 firmware during HIL execution.

The visualization layer exists to make this system observable during demonstrations and validation runs.

The intended live chain is:

```text
                       Linux PC
        ┌──────────────────────────────────┐
        │            hil_runner            │
        │                                  │
        │  Aircraft / Physics Model        │
        │  Sensor Simulation               │
        │  Fault Injection                 │
        │  Real-Time HIL Scheduler         │
        │  Telemetry / Event Collection    │
        └───────────────┬──────────────────┘
                        │
                        │ physical transport
                        ▼
                 ┌─────────────┐
                 │ STM32 FC1   │
                 │   Zephyr    │
                 └──────┬──────┘
                        │
                   FC1 ↔ FC2
                        │
                 ┌──────▼──────┐
                 │ STM32 FC2   │
                 │   Zephyr    │
                 └─────────────┘

        hil_runner ── telemetry ──> Digital Twin Viewer
```

The viewer must remain a **consumer of telemetry**, never the authority controlling the simulation or flight-control loop.

---

## 2. Design goals

The viewer should make the HIL execution understandable within a few seconds during a technical demonstration.

It should show:

- aircraft position and attitude;
- mission state;
- altitude and position targets;
- actuator state;
- FC1 and FC2 status;
- health and safety state;
- active fault and detection events;
- HIL timing information;
- the health of relevant logical/electronic components;
- a short live event timeline.

The viewer is intentionally **not** intended to become a complete ground-control station, simulator, or flight-data analysis suite.

The priority is clarity.

---

## 3. Architectural rules

### 3.1 Single source of truth

`hil_runner` remains responsible for:

- the aircraft model;
- simulation time;
- real-time scheduling;
- communication with the STM32 targets;
- fault injection;
- telemetry generation;
- safety and mission observations.

The frontend must not implement a second physics model.

If visualization data stops arriving, the frontend must indicate that telemetry is stale or disconnected. It must not predict or invent the continuation of the flight.

### 3.2 Visualization must never disturb HIL timing

The control loop may execute at a significantly higher frequency than the graphical interface.

For example, if the HIL control loop runs at 200 Hz, the viewer does not need 200 telemetry updates per second.

Recommended architecture:

```text
HIL control loop     : project-configured real-time frequency
Viewer publication   : 20–30 Hz
Browser rendering    : up to 60 FPS
```

The telemetry publisher must therefore be **non-blocking** from the HIL runner's point of view.

If:

- the browser is closed;
- no client is connected;
- the frontend is slow;
- the WebSocket connection fails;

then the HIL execution must continue normally.

Dropping a visualization update is preferable to delaying a control deadline.

---

## 4. Viewer technology

Recommended stack:

```text
React
TypeScript
React Three Fiber
Three.js
WebSocket over localhost
```

React is useful for the surrounding avionics/status interface, while React Three Fiber / Three.js handles the 3D scene.

No database is required.

No Python backend is required unless the current project architecture later provides a concrete reason for one.

The Linux `hil_runner` should expose the live visualization stream directly or through a very small dedicated telemetry-publisher component.

---

## 5. Telemetry contract

Create one compact message dedicated to visualization, conceptually named:

```text
TwinSnapshot
```

It should be assembled from existing project state rather than introducing new control state.

A suitable structure is:

```text
TwinSnapshot
├── timestamp
├── aircraft
│   ├── position
│   ├── velocity
│   ├── pitch
│   └── roll
├── sensors
├── actuators
├── mission
├── health
├── safety
├── fc1
├── fc2
├── fault
└── hil_timing
```

The exact fields must follow the structures already present in the repository.

A JSON representation may initially be used for simplicity, for example:

```json
{
  "time_s": 12.45,
  "aircraft": {
    "x_m": 4.1,
    "y_m": -1.2,
    "z_m": 99.7,
    "pitch_rad": 0.03,
    "roll_rad": -0.02
  },
  "actuators": {
    "rotor_rpm": 5350,
    "left_servo_deg": 1.3,
    "right_servo_deg": -0.8
  },
  "mission": "STATION_KEEPING",
  "health": "HEALTHY",
  "safety_mode": "NORMAL",
  "fc1": "ONLINE",
  "fc2": "ONLINE",
  "active_fault": null,
  "hil": {
    "deadline_misses": 0
  }
}
```

The viewer does not need to emphasize the distinction between simulation ground truth and sensor observation in its normal presentation. However, the data model should preserve that distinction where it already exists so that sensor-fault demonstrations remain technically correct.

---

## 6. Main 3D aircraft view

The central area of the application displays the aircraft in a deliberately simple environment.

Required elements:

- aircraft model;
- X/Y position;
- altitude;
- pitch;
- roll;
- ground/grid reference;
- station-keeping zone;
- target position;
- target altitude;
- recent trajectory trail.

Optional if already available in telemetry:

- wind vector;
- rotor/wing animation based on RPM;
- actuator/servo visualization.

Do not spend project time producing a photorealistic environment.

A clean engineering visualization is preferable to a game-like scene.

---

## 7. 3D asset strategy

The user should not need to manually model assets in Blender.

Two different strategies are recommended depending on the asset.

### 7.1 Aircraft model — AI-generated GLB

The aircraft is mostly a visual object. An AI-generated 3D asset is appropriate here.

Recommended workflow:

```text
text prompt / reference image
            ↓
AI 3D generator
            ↓
GLB
            ↓
Three.js GLTFLoader / React Three Fiber
```

Suitable current tools include:

- **Meshy** — text-to-3D and image-to-3D with GLB export;
- **Tripo** — text/image-to-3D and GLB/GLTF-oriented workflows;
- **Sloyd** — text/image-to-3D with GLB export.

For this project, the generated aircraft does not need to match a real Heliblade vehicle. It should visually communicate the simplified project aircraft and must not imply that it represents X721's proprietary design.

A useful generation prompt would describe something such as:

> Minimal lightweight experimental high-altitude drone, symmetric twin rotating-wing concept, engineering prototype, clean topology, low-poly real-time asset, no weapons, no landing gear emphasis, neutral materials, isolated object.

The model should be kept lightweight enough for browser rendering.

### 7.2 Flight-controller board — procedural 3D, not a monolithic AI GLB

For the electronic board view, a monolithic AI-generated GLB is **not recommended**.

The viewer needs to independently change the state of individual components. A generated mesh may not provide predictable component names, hierarchy, or separation.

Instead, construct a simplified board procedurally in React Three Fiber from basic meshes.

Conceptually:

```text
Nucleo board
├── PCB
├── MCU
├── communication interface
├── sensor interface
├── actuator interface
├── clock / scheduler indicator
└── power / platform block
```

The representation does not need to reproduce every resistor or trace on the real Nucleo-L476RG.

It is an **architectural health view**, not an electrical CAD model.

Each component should have a stable logical identifier, for example:

```text
fc1.mcu
fc1.transport
fc1.sensor_input
fc1.control
fc1.actuator_output

fc2.mcu
fc2.transport
fc2.supervision
fc2.safety_manager
```

This allows telemetry to update individual component materials directly.

Example state mapping:

```text
HEALTHY     → normal/green indication
DEGRADED    → yellow indication
FAILED/SAFE → red indication when semantically appropriate
UNKNOWN     → neutral/grey indication
```

The exact meaning of red/yellow must follow the project's existing `HealthState`, `SafetyMode`, and fault vocabulary rather than creating a new safety taxonomy solely for the UI.

An AI coding agent can generate this geometry directly from React Three Fiber primitives, so no manual 3D modelling step is required.

---

## 8. Electronic / FC health panel

The viewer should contain a small 3D avionics panel next to the aircraft view.

Suggested arrangement:

```text
┌───────────────────────┐
│ FC1                    │
│   [ MCU ]              │
│ [COM] [CTRL] [SENSORS] │
│       [ACTUATORS]      │
└───────────────────────┘

┌───────────────────────┐
│ FC2                    │
│   [ MCU ]              │
│ [COM] [SUPERVISION]    │
│     [SAFETY]           │
└───────────────────────┘
```

A component changes appearance when its associated state changes.

Examples:

```text
Communication degradation
    → communication block becomes yellow

Communication timeout
    → communication block becomes red
    → SAFE / SAFE_MODE is shown separately

Invalid altitude sensor
    → sensor block becomes yellow

Actuator degradation
    → actuator block becomes yellow
```

Do not attempt to infer physical silicon damage from a software failure mode.

The panel visualizes the **logical subsystem affected by the detected failure**, not a literal hardware diagnosis unless the software genuinely provides one.

---

## 9. Main interface layout

Recommended layout:

```text
┌──────────────────────────────────────────────────────────────┐
│ HIL LIVE       FC1 ●       FC2 ●       timing / miss count │
├──────────────────────────────────────┬───────────────────────┤
│                                      │ Mission               │
│                                      │ STATION_KEEPING       │
│                                      │                       │
│             AIRCRAFT 3D              │ Health                │
│                                      │ HEALTHY               │
│                                      │                       │
│                                      │ Safety                │
│                                      │ NORMAL                │
│                                      │                       │
│                                      │ Active fault          │
│                                      │ NONE                  │
├───────────────────────────────┬──────┴───────────────────────┤
│ FC1 / FC2 3D HEALTH VIEW      │ Altitude / position graphs  │
├───────────────────────────────┴──────────────────────────────┤
│ Event timeline                                                │
└──────────────────────────────────────────────────────────────┘
```

The interface should make the following immediately visible:

```text
What is the aircraft doing?
What mission phase is active?
Are FC1 and FC2 alive?
Is the system healthy?
Has a fault been injected/detected?
What safety response occurred?
Is the HIL loop meeting its timing constraints?
```

---

## 10. Graphs

Keep the graphs intentionally limited.

Initial version:

1. **Altitude vs target**
2. **Position error vs time**

Additional graphs should only be added if they improve a concrete demonstration.

The project already contains structured telemetry for deeper analysis; the live viewer does not need to reproduce an offline plotting suite.

---

## 11. Fault visualization

Two demonstration flows should receive special attention because they already correspond to the project's validation strategy.

### 11.1 Recoverable degradation

Example:

```text
INVALID_SENSOR_DATA
        ↓
SENSOR_VALIDATION_FAILED
        ↓
DEGRADED
        ↓
COMPENSATED
        ↓
RECOVERY
        ↓
NORMAL
```

Expected viewer behavior:

- affected sensor/interface block becomes yellow;
- active fault is displayed;
- `DEGRADED` and `COMPENSATED` become clearly visible;
- aircraft continues to evolve normally according to the Digital Twin;
- recovery transition is shown in the event timeline;
- component returns to healthy appearance once the existing system declares recovery.

### 11.2 Mission abort / safe mode

Example:

```text
FC / COMMUNICATION FAILURE
        ↓
SUPERVISION TIMEOUT
        ↓
SAFE
        ↓
SAFE_MODE
        ↓
CONTROLLED DESCENT
        ↓
ABORTED
```

Expected viewer behavior:

- affected FC/communication block becomes red;
- FC link state visibly changes;
- SAFE and SAFE_MODE are prominent;
- mission state becomes ABORTED when the existing mission logic reports it;
- the aircraft view visibly follows the commanded safe behavior.

The viewer must display existing system decisions. It must not implement its own safety decisions.

---

## 12. Event timeline

Display only important events in the main interface.

Examples:

```text
12.000  MISSION       STATION_KEEPING
15.000  FAULT         INVALID_SENSOR_DATA
15.010  DETECTION     SENSOR_VALIDATION_FAILED
15.010  HEALTH        DEGRADED
15.010  SAFETY        COMPENSATED
18.000  RECOVERY      SENSOR_VALID
18.010  HEALTH        HEALTHY
18.010  SAFETY        NORMAL
```

Do not render every heartbeat or every high-frequency packet in the human-readable event timeline.

Raw transport traces may remain available in existing logs.

---

## 13. Live and replay modes

The same viewer should support two data sources.

### Live

```text
hil_runner
    ↓
WebSocket
    ↓
viewer
```

This is the primary demonstration mode with the physical STM32 boards connected.

### Replay

```text
recorded TwinSnapshot stream
             ↓
           viewer
```

Replay provides:

- deterministic demonstrations;
- debugging;
- visual inspection of previous runs;
- a fallback if physical hardware is unavailable during a presentation.

Replay must reproduce recorded data rather than re-simulating the flight in JavaScript.

---

## 14. Implementation plan

### DT-1 — Telemetry contract

Create `TwinSnapshot` from existing project structures.

Do not alter control logic merely for visualization.

### DT-2 — Non-blocking publisher

Add a visualization output to `hil_runner`.

Requirements:

- localhost WebSocket;
- approximately 20–30 Hz publication;
- no impact on HIL deadlines;
- no dependency on viewer availability.

### DT-3 — Minimal viewer

Implement:

- WebSocket connection;
- connection status;
- simple aircraft object;
- x/y/z;
- pitch/roll;
- mission state.

### DT-4 — Full status panel

Add:

- health;
- safety mode;
- active fault;
- FC1/FC2 state;
- timing/deadline information;
- actuator values.

### DT-5 — Aircraft asset

Replace the primitive aircraft with an AI-generated lightweight GLB if this improves presentation quality.

### DT-6 — FC board health view

Implement the procedural 3D FC1/FC2 board representation with independently addressable logical components.

### DT-7 — Environment

Add:

- station-keeping zone;
- target;
- trajectory trail;
- ground/grid;
- optional wind indication.

### DT-8 — Graphs and event timeline

Add the two key graphs and filtered event history.

### DT-9 — Replay

Record and replay `TwinSnapshot` streams through the same viewer state pipeline.

### DT-10 — Demonstration validation

Run at least:

- nominal HIL mission;
- recoverable sensor fault;
- communication/FC failure leading to safe mode;
- replay of at least one recorded run.

Verify that closing or slowing the viewer cannot affect the HIL control loop.

---

## 15. Out of scope

The Digital Twin viewer should not introduce:

- a second flight-dynamics implementation;
- a database;
- cloud infrastructure;
- ROS 2 solely for visualization;
- Unity or Unreal solely for presentation;
- a full ground-control station;
- photorealistic scenery;
- a detailed electrical CAD model of the Nucleo board;
- new fault semantics that do not exist in the flight software;
- new aircraft axes or systems such as yaw or battery simulation unless added independently to the actual project.

---

## 16. Definition of done

The Digital Twin visualization is complete when:

```text
[ ] hil_runner remains the authoritative HIL process
[ ] FC1 and FC2 physical firmware continue to run independently of the viewer
[ ] live TwinSnapshot telemetry reaches the viewer
[ ] visualization cannot block the HIL real-time loop
[ ] aircraft position / altitude / pitch / roll are visible
[ ] station-keeping target and trajectory are visible
[ ] mission state is visible
[ ] health and safety mode are visible
[ ] active faults and detections are visible
[ ] FC1 / FC2 connectivity is visible
[ ] FC subsystem health can be represented in the 3D board panel
[ ] altitude and position-error plots are available
[ ] important events are displayed without heartbeat spam
[ ] recoverable degradation is demonstrable
[ ] safe-mode / mission-abort scenario is demonstrable
[ ] replay mode works from a recorded run
[ ] the viewer does not implement flight physics or safety logic
```
