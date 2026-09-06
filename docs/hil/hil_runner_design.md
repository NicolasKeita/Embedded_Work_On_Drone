# HIL Runner & Real-Time Closed-Loop Mission — Design

## Status

> **HIL infrastructure and host-target closed-loop validation implemented and ready
> for physical STM32 integration.** No physical STM32 is available yet. The
> host-side FC emulator (`fc1_hil_host`) exercises the same Flight Controller core
> that will later run on the STM32 target over the very same HIL-Proto wire path.
> It is **not** a real-hardware HIL run.

## Component separation

| Layer | Where | Role |
|---|---|---|
| HIL runner / orchestrator | `Src/Embedded/Hil/HilRunner*` | Loads scenario, owns the aircraft simulator, paces real time, exchanges SensorPacket/ActuatorPacket over the transport, records telemetry/events, monitors deadlines, computes verdict. Independent of physics internals and FC internals. |
| Aircraft simulator | `Src/Simulation/Aircraft` (shared) | Evolves from actuator commands — never replayed. |
| HIL transport | `Src/Embedded/Hil/HilTransport` + `Src/Embedded/Transport/*` | Framing/codec/parser over `ITransport`. Replaceable byte channel (loopback host / OS pipe / future UART). |
| FC target interface | `HilFcTarget` (`IFcTarget`) | The thing on the far end of the transport. |
| Host FC emulator | `fc1_hil_host` exe + `HostFcTarget` | Runs the **real** `sim::control::FlightController` core (`Src/Control/*`), the same core the future `fc1_stm32` firmware will use — not a HIL-specific algorithm. |
| **REAL HIL TARGET (future)** | `fc1_stm32` on STM32 | Drop-in replacement of `fc1_hil_host`; only the transport (`serial`) and the target process change — the runner and the aircraft stay untouched. |
| Safety / health / fault / telemetry cores | `Src/Safety/*`, `Src/SIL/Core/{CommsBus,Telemetry,Faults...}` (shared) | Reused verbatim by the HIL runner so the detection chain matches the validated SIL baseline. |

## Closed loop (one step)

```
read aircraft ground truth (private to PC)
  -> sensor model -> HAL::SensorData (measurement, never truth)
  -> SensorPacket (HIL-Proto) -> transport
  -> [HostFcTarget / future STM32] decode -> FlightController::update -> encode
  -> ActuatorPacket -> transport
  -> runner decode/validate (sequence, timestamp echo, deadline)
  -> apply actuator (efficiency) -> Aircraft::update(dt)
  -> record truth + sensor telemetry, events, timing
  -> schedule next step on an absolute wall-clock deadline
```

## Time bases (kept separate)

* `sim_timestamp_us` — simulation/aircraft time, advanced by the control period each step.
* wall clock — `std::chrono::steady_clock` (monotonic), used only for pacing, transport latency and deadlines. Never used to claim "0 us" comm latency.

## Real-time pacing (absolute, drift-free)

```
next_deadline = start + period      // wall clock, microseconds
for each step:
    run one closed-loop exchange
    sleep until next_deadline if early            // absolute, not `sleep(period)`
    if now > next_deadline: record DEADLINE_MISSED (+policy)
    next_deadline += period                       // carry overruns, no drift accumulation
```

## Timestep

The project control rate is **100 Hz / 10 ms** (`hil_protocol.md` §1.3 ControlTask,
mirroring `SilConfig::dt = 0.01`). `dt = 0.01 s` ⇒ 30 s mission = **3000 steps**
(computed from configuration, never hard-coded).

## Deadline policy

`Warn` (default, conservative) records and continues; `Fail` flags the verdict;
`Abort` terminates the loop early. Policy is configurable per scenario.
