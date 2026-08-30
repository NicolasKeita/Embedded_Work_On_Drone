/*
Filename: Src/SIL/SilRunner.cppm
Description: Public interface of the SIL orchestration engine running the fault injection pipeline.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilRunner;

import std;

import Aircraft;
import FlightController;
import Telemetry;

import CommsBus;
import FaultInjectors;
import HealthMonitor;
import SafetyManager;
import SilTypes;

export namespace sim::sil {

using sim::safety::HealthMonitor;
using sim::safety::SafetyManager;

// Typed failures of a SIL run (no exception is ever thrown).
enum class SilError { TooManyScenarios, FaultScenarioRejected };

// Maximum number of fault scenarios a single run can carry (fixed capacity).
inline constexpr std::size_t kMaxFaultInjectors = 8;

struct SilConfig {
    double dt = 0.01;
    double duration_s = 60.0;
    sim::control::TargetState target{.z = 10.0};
    sim::control::ControllerConfig controller{.hover_rpm = Aircraft{}.hover_rpm()};
    double heartbeat_timeout_s = 0.10;
    double actuator_mismatch_rpm = 60.0;
    double thrust_compensation_margin = 1.7;
    double safe_descent_rpm_rate = 4000.0;
    SensorValidationLimits sensor_limits{};
    std::uint64_t seed = 42;
};

/*
Orchestrateur SIL : boucle temporelle synchrone a pas constant reliant toute la
chaine Aircraft -> FaultInjector -> Sensors/Comms -> FC1/FC2 -> HealthMonitor ->
SafetyManager -> Actuators.
*/
class SILRunner {
public:
    explicit SILRunner(SilConfig config = {});

    /*
    Runs the full simulation for the given fault scenarios and returns the
    result, or a typed error when the scenario batch cannot be set up.
    */
    [[nodiscard]] std::expected<SimulationResult, SilError> run(std::span<const FaultScenario> scenarios);

private:
    struct RunContext {
        explicit RunContext(const SilConfig& cfg);

        SilConfig config;
        Aircraft aircraft;
        sim::control::FlightController fc1;
        CommsBus comms;
        HealthMonitor health;
        SafetyManager safety;
        sim::safety::SafetyCommand safety_command{};
        std::array<FaultInjector, kMaxFaultInjectors> injectors{};
        std::size_t injector_count = 0;
        SimulationState env{};
        ControlCommand command{};
        AircraftState fc1_view{};
        SensorTelemetry telemetry{};
        double commanded_rpm = 0.0;
        double last_effective_rpm = 0.0;
        double safe_rpm = 0.0;
        double time = 0.0;
        SimulationResult result{};
    };

    [[nodiscard]] static std::expected<RunContext, SilError> make_context(const SilConfig&                     config,
                                                                          std::span<const FaultScenario> scenarios);
    static void execute(RunContext& ctx);
    static void apply_injectors(RunContext& ctx);
    static void update_fc1(RunContext& ctx);
    static void update_monitoring(RunContext& ctx);
    static void apply_actuators(RunContext& ctx);
    static void update_metrics(RunContext& ctx);
    static void finalize(RunContext& ctx);

    SilConfig config_;
};

}
