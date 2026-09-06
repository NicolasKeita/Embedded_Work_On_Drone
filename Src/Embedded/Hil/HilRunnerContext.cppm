/*
Filename: Src/Embedded/Hil/HilRunnerContext.cppm
Description: Run context, error modes, result and structured output of the HIL runner.
Wires the aircraft simulator, the reused SIL safety/health/comms/fault cores, the HIL
sensor model, the FC target, the transport and the wall clock, plus the per-run cached
values used by the event recorders. Holds sim::sil::SimulationState and
sim::sil::FaultInjector to reuse the same fault-injection vocabulary as the validated
SIL baseline, with no behavioural fork.
Export summary: ClockKind, HilError, HilResult, HilRunOutput, HilRunContext.

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
import LoopbackTransport;
import SafetyManager;
import SilFaultScenario;
import SilTypes;
import Telemetry;
import Transport;

export namespace sim::hil {

enum class HilError {
    TooManyScenarios,
    FaultScenarioRejected,
    InvalidConfiguration
};

// Maximum number of fault scenarios a single HIL run can carry (fixed capacity, matches
// the SIL baseline limit).
inline constexpr std::size_t kHilMaxFaultScenarios = 8;

struct HilResult {
    bool                       mission_success = false;
    sim::control::MissionState final_state = sim::control::MissionState::TAKEOFF;
    std::float64_t             mission_end_time = -1.0;

    sim::safety::HealthState   final_health = sim::safety::HealthState::HEALTHY;
    sim::safety::SafetyMode    final_safety_mode = sim::safety::SafetyMode::NORMAL;
    bool                       degraded_reached = false;
    bool                       compensated_reached = false;
    bool                       safe_mode_reached = false;
    sim::safety::FaultDomain   first_fault_domain = sim::safety::FaultDomain::FC1Heartbeat;

    sim::sil::FaultType        fault_type = sim::sil::FaultType::None;
    bool                       fault_detected = false;
    std::float64_t             fault_injected_time = -1.0;
    std::float64_t             detection_time = -1.0;
    std::float64_t             safety_response_time = -1.0;
    std::float64_t             detection_latency = -1.0;
    std::float64_t             response_latency = -1.0;

    std::float64_t             max_position_error_m = 0.0;
    std::float64_t             max_altitude_error_m = 0.0;
    std::float64_t             final_x_m = 0.0;
    std::float64_t             final_y_m = 0.0;
    std::float64_t             final_altitude_m = 0.0;
    std::float64_t             max_pitch_rad = 0.0;
    std::float64_t             max_roll_rad = 0.0;

    HilCommStats               comms{};
    HilTimingStats             timing{};
    bool                       real_time_pacing = true;
    std::string_view           scenario_id{};
    bool                       fault_expected = false;
    bool                       test_verdict = false;

    [[nodiscard]] bool compute_verdict(bool fault_expected) const noexcept;
};

struct HilRunOutput {
    HilResult                    result{};
    HilConfig                    config{};
    std::vector<HilEvent>        events{};
    std::vector<HilSensorSample> telemetry{};
    std::vector<HilTruthSample>  ground_truth{};
};

/*
Full mutable state of one HIL run: configuration, aircraft, the reused safety/health/
comms cores, the HIL sensor model, the FC target, the transport over a loopback byte
channel and the wall clock, plus the cached values used by the event recorders.
*/
struct HilRunContext {
    explicit HilRunContext(const HilConfig& cfg);

    const HilConfig                                config;
    ClockKind                                      clock_kind;
    Aircraft                                       aircraft;
    sim::sil::CommsBus                             comms;
    sim::safety::HealthMonitor                     health;
    sim::safety::SafetyManager                     safety;
    sim::safety::SafetyCommand                     safety_command{};
    HilSensorModel                                 sensor_model;
    FlightCore::Sim::LoopbackTransport             channel;
    HilTransport                                   transport;
    std::unique_ptr<IFcTarget>                     fc_target;
    std::unique_ptr<IWallClock>                    clock;

    std::array<sim::sil::FaultInjector, kHilMaxFaultScenarios> injectors{};
    std::size_t                                    injector_count = 0;
    sim::sil::SimulationState                      env{};

    sim::sil::SensorTelemetry                      sensors{};
    AircraftState                                  sampled_truth{};
    FlightCore::HAL::SensorData                    last_sensor_data{};
    FlightCore::HAL::ActuatorCommands              actuator_cmd{};
    ControlCommand                                 command{};
    HilResult                                      result{};
    HilTrace                                       trace;
    HilTelemetryRecorder                           telemetry_recorder{};
    HilTimingStats                                 timing{};

    sim::control::MissionState                     previous_mission_state = sim::control::MissionState::TAKEOFF;
    sim::safety::SafetyMode                        previous_safety_mode = sim::safety::SafetyMode::NORMAL;
    sim::safety::HealthState                       previous_health = sim::safety::HealthState::HEALTHY;
    bool                                           fault_active = false;
    bool                                           fault_recorded = false;
    bool                                           detection_recorded = false;
    bool                                           safety_response_recorded = false;
    bool                                           mission_abort_recorded = false;
    bool                                           fc1_was_alive = true;
    sim::sil::FaultType                            last_fault_type = sim::sil::FaultType::None;

    std::uint64_t                                  step = 0;
    std::float64_t                                 time = 0.0;
    std::uint64_t                                  sim_ts_us = 0;
    std::uint64_t                                  start_wall_us = 0;
    std::uint64_t                                  next_deadline_us = 0;
    std::int64_t                                   this_rtt_us = -1;
    bool                                           this_received = false;
    std::uint64_t                                  sensor_send_wall_us = 0;
    std::uint64_t                                  actuator_receive_wall_us = 0;
    FcStepOutcome                                  this_fc{};

    std::float64_t                                 commanded_rpm = 0.0;
    std::float64_t                                 last_effective_rpm = 0.0;
    std::float64_t                                 safe_rpm = 0.0;

    bool                                           fault_expected = false;
    bool                                           aborted_on_deadline = false;

    std::ostream*                                  live_out = nullptr;
    std::size_t                                    live_event_cursor = 0;
    std::uint64_t                                  live_report_index = 0;
};

}
