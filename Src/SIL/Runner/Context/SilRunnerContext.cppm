/*
Filename: Src/SIL/Runner/Context/SilRunnerContext.cppm
Description: Run context of the SIL runner: configuration, structured output, error modes and per-run state.
Exports:
    enum class SilError,
    struct SilConfig,
    struct SilRunOutput,
    struct RunContext

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunnerContext;

import std;

import Aircraft;
import CommsBus;
import FaultInjectors;
import FlightController;
import HealthMonitor;
import SafetyManager;
import SilEvents;
import SilTelemetry;
import SilTypes;
import Telemetry;

export namespace sim::sil {

// Maximum number of fault scenarios a single run can carry (fixed capacity).
inline constexpr std::size_t kMaxFaultInjectors = 8;

/*
Typed failures of a SIL run (infrastructure-domain errors: scenario list
capacity and scenario validation; no exception is ever thrown).
*/
enum class SilError { TooManyScenarios, FaultScenarioRejected };

struct SilConfig {
    std::float64_t                 dt = 0.01;
    std::float64_t                 duration_s = 30.0;
    sim::control::TargetState      target{.z = 10.0};
    sim::control::ControllerConfig controller{.hover_rpm = Aircraft{}.hover_rpm()};
    std::float64_t                 heartbeat_timeout_s = 0.10;
    std::float64_t                 actuator_mismatch_rpm = 60.0;
    std::float64_t                 thrust_compensation_margin = 1.7;
    SensorValidationLimits         sensor_limits{};
    std::uint64_t                  seed = 42;
    std::float64_t                 transport_latency_s = 0.004;
    std::float64_t                 telemetry_rate_hz = 20.0;
    SilLogLevel                    trace_level = SilLogLevel::Info;
};

// Full observable output of one SIL run: aggregates plus trace artifacts.
struct SilRunOutput {
    SimulationResult             result{};
    std::vector<SilEvent>        events{};
    std::vector<TelemetrySample> telemetry{};
    std::vector<TrueStateSample> ground_truth{};
};

/*
Full mutable state of one SIL run: configuration, simulated aircraft and
subsystems, value injectors, trace and telemetry recorders, plus the cached
values used by the event recorders.
*/
struct RunContext {
    explicit RunContext(const SilConfig& cfg);

    SilConfig                                           config;
    Aircraft                                            aircraft;
    sim::control::FlightController                      fc1;
    CommsBus                                            comms;
    sim::safety::HealthMonitor                          health;
    sim::safety::SafetyManager                          safety;
    sim::safety::SafetyCommand                          safety_command{};
    std::array<FaultInjector, kMaxFaultInjectors>       injectors{};
    std::size_t                                         injector_count = 0;
    SimulationState                                     env{};
    ControlCommand                                      command{};
    AircraftState                                       fc1_view{};
    AircraftState                                       sampled_truth{};
    SensorTelemetry                                     telemetry{};
    SimulationResult                                    result{};
    SilTrace                                            trace;
    TelemetryRecorder                                   telemetry_recorder{};
    CommsStats                                          comms_stats{};
    std::array<bool, sim::safety::kDetectionEventCount> previous_flags{};
    sim::control::MissionState                          previous_mission_state = sim::control::MissionState::SPIN_UP;
    sim::safety::SafetyMode                             previous_safety_mode = sim::safety::SafetyMode::NORMAL;
    sim::safety::HealthState                            previous_health = sim::safety::HealthState::HEALTHY;
    bool                                                fc1_was_alive = true;
    bool                                                fault_active = false;
    bool                                                fault_recorded = false;
    bool                                                detection_recorded = false;
    bool                                                recovery_recorded = false;
    bool                                                safety_response_recorded = false;
    bool                                                mission_abort_recorded = false;
    FailureMode                                         last_failure_mode = FailureMode::NONE;
    FaultTarget                                         last_fault_target = FaultTarget::Unspecified;
    std::float64_t                                      last_fault_start = 0.0;
    std::float64_t                                      commanded_rpm = 0.0;
    std::float64_t                                      last_effective_rpm = 0.0;
    std::float64_t                                      time = 0.0;
    std::float64_t                                      mission_end_time = -1.0;
    std::float64_t                                      position_error_sum = 0.0;
    std::float64_t                                      altitude_error_sum = 0.0;
    std::uint64_t                                       metric_samples = 0;
};

}
