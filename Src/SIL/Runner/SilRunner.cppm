/*
Filename: Src/SIL/Runner/SilRunner.cppm
Description: Public interface of the SIL orchestration engine and its fixed-capacity run context.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunner;

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

export import :Context;

export namespace sim::sil {
// Maximum number of fault scenarios a single run can carry (fixed capacity).
inline constexpr std::size_t kMaxFaultInjectors = 8;

// SIL orchestrator: fixed-time-step loop linking Aircraft -> FaultInjector ->
// Sensors/Comms -> FC1/FC2 -> HealthMonitor -> SafetyManager -> Actuators.
// Logging is purely observational: events and telemetry never alter the run.
class SILRunner {
public:
    explicit SILRunner(SilConfig config = {});

    // Runs the scenarios and returns the structured result and trace artifacts.
    [[nodiscard]] std::expected<SilRunOutput, SilError> run(std::span<const FaultScenario> scenarios);

private:
    struct RunContext {
        explicit RunContext(const SilConfig& cfg);

        SilConfig                                     config;
        Aircraft                                      aircraft;
        sim::control::FlightController                fc1;
        CommsBus                                      comms;
        sim::safety::HealthMonitor                    health;
        sim::safety::SafetyManager                    safety;
        sim::safety::SafetyCommand                    safety_command{};
        std::array<FaultInjector, kMaxFaultInjectors> injectors{};
        std::size_t                                   injector_count = 0;
        SimulationState                               env{};
        ControlCommand                                command{};
        AircraftState fc1_view{}, sampled_truth{};
        SensorTelemetry            telemetry{};
        SimulationResult           result{};
        SilTrace                   trace;
        TelemetryRecorder          telemetry_recorder{};
        CommsStats                 comms_stats{};
        std::array<bool, 4>        previous_flags{};
        sim::control::MissionState previous_mission_state = sim::control::MissionState::TAKEOFF;
        sim::safety::SafetyMode    previous_safety_mode = sim::safety::SafetyMode::NORMAL;
        sim::safety::HealthState   previous_health = sim::safety::HealthState::HEALTHY;
        bool fc1_was_alive = true, fault_active = false, fault_recorded = false, detection_recorded = false;
        bool recovery_recorded = false, safety_response_recorded = false;
        bool           mission_abort_recorded = false;
        FaultType      last_fault_type = FaultType::None;
        std::float64_t last_fault_start = 0.0;
        std::float64_t commanded_rpm = 0.0, last_effective_rpm = 0.0, safe_rpm = 0.0;
        std::float64_t time = 0.0, mission_end_time = -1.0, position_error_sum = 0.0, altitude_error_sum = 0.0;
        std::uint64_t metric_samples = 0;
    };
    static std::expected<RunContext, SilError> make_context(const SilConfig&, std::span<const FaultScenario>);
    static void execute(RunContext&);
    static void apply_injectors(RunContext&);
    static void update_fc1(RunContext&);
    static void update_monitoring(RunContext&);
    static void apply_actuators(RunContext&);
    static void update_metrics(RunContext&);
    static void finalize(RunContext&);

    static void record_run_start(RunContext&), record_run_end(RunContext&), record_fc1_failure(RunContext&);
    static void record_heartbeat(RunContext&, const CommsDelivery&);
    static void record_heartbeat_delivered(RunContext&, const CommsDelivery&);
    static void record_heartbeat_dropped(RunContext&, const CommsDelivery&);
    static void record_watchdog_and_detection(RunContext&, const sim::safety::HealthReport&);
    static void record_watchdog_events(RunContext&, const sim::safety::HealthReport&);
    static void record_recovery_start(RunContext&, const sim::safety::HealthReport&);
    static void record_detection(RunContext&, const sim::safety::HealthReport&);
    static void record_health_transition(RunContext&, sim::safety::HealthState);
    static void record_recovery_end(RunContext&, sim::safety::HealthState);
    static void record_safety_transitions(RunContext&);
    static void record_mission_transition(RunContext&, sim::control::MissionState);
    static void record_fault_activation(RunContext&, const FaultScenario&), record_fault_cleared(RunContext&);

    SilConfig config_;
};

}
