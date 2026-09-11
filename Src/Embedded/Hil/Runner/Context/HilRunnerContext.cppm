/*
Filename: Src/Embedded/Hil/Runner/Context/HilRunnerContext.cppm
Description: Run context of the HIL runner (error modes, result and structured output
types live in the HilRunnerTypes module, re-exported below). Wires the aircraft
simulator, the reused SIL safety/health/comms/fault cores, the HIL sensor model, the
FC target, the transport and the wall clock, plus the per-run cached values used by
the event recorders. Holds sim::sil::SimulationState and sim::sil::FaultInjector to
reuse the same fault-injection vocabulary as the validated SIL baseline.
Export summary: HilRunContext (plus HilRunnerTypes re-export).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module HilRunnerContext;

import std;

import Aircraft;
import CommsBus;
import FaultInjectors;
import FlightController;
import FlightControllerTypes;
import HalTypes;
import HealthMonitor;
import HilClock;
import HilConfig;
import HilEvents;
import HilFcTarget;
import HilProtocol;
import HilSensorModel;
import HilTelemetry;
import HilTiming;
import HilTransport;
import SafetyManager;
import SilFaultScenario;
import SilTypes;
import Telemetry;
import Transport;

export import HilRunnerTypes;

export namespace sim::hil {

/*
Full mutable state of one HIL run: configuration, aircraft, the reused safety/health/
comms cores, the HIL sensor model, the FC target, the transport over a loopback byte
channel and the wall clock, plus the cached values used by the event recorders.
*/
struct HilRunContext {
    HilRunContext(const HilConfig& cfg, std::unique_ptr<FlightCore::Transport::ITransport> byte_channel);

    const HilConfig                                    config;
    Aircraft                                           aircraft;
    sim::sil::CommsBus                                 comms;
    sim::safety::HealthMonitor                         health;
    sim::safety::SafetyManager                         safety;
    sim::safety::SafetyCommand                         safety_command{};
    HilSensorModel                                     sensor_model;
    std::unique_ptr<FlightCore::Transport::ITransport> channel;
    HilTransport                                       transport;
    std::unique_ptr<IFcTarget>                         fc_target;
    MonotonicClock                                     clock;

    std::array<sim::sil::FaultInjector, kHilMaxFaultScenarios> injectors{};
    std::size_t                                                injector_count = 0;
    sim::sil::SimulationState                                  env{};

    sim::sil::SensorTelemetry         sensors{};
    AircraftState                     sampled_truth{};
    FlightCore::HAL::SensorData       last_sensor_data{};
    FlightCore::HAL::ActuatorCommands actuator_cmd{};
    ControlCommand                    command{};
    HilResult                         result{};
    HilTrace                          trace;
    HilTelemetryRecorder              telemetry_recorder{};
    HilTimingStats                    timing{};

    sim::control::MissionState previous_mission_state = sim::control::MissionState::SPIN_UP;
    sim::safety::SafetyMode    previous_safety_mode = sim::safety::SafetyMode::NORMAL;
    sim::safety::HealthState   previous_health = sim::safety::HealthState::HEALTHY;
    bool                       fault_active = false;
    bool                       fault_recorded = false;
    bool                       detection_recorded = false;
    bool                       safety_response_recorded = false;
    bool                       mission_abort_recorded = false;
    bool                       fc1_was_alive = true;
    sim::sil::FailureMode      last_failure_mode = sim::sil::FailureMode::NONE;

    std::uint64_t  step = 0;
    std::float64_t time = 0.0;
    std::uint64_t  sim_ts_us = 0;
    std::uint64_t  start_wall_us = 0;
    std::uint64_t  next_deadline_us = 0;
    std::int64_t   this_rtt_us = -1;
    bool           this_received = false;
    std::uint64_t  sensor_send_wall_us = 0;
    std::uint64_t  actuator_receive_wall_us = 0;
    FcStepOutcome  this_fc{};

    std::float64_t commanded_rpm = 0.0;
    std::float64_t last_effective_rpm = 0.0;
    std::float64_t safe_rpm = 0.0;

    bool fault_expected = false;
    bool aborted_on_deadline = false;

    std::ostream* live_out = nullptr;
    std::size_t   live_event_cursor = 0;
    std::size_t   live_telemetry_cursor = 0;
    std::uint64_t live_report_index = 0;
};

}
