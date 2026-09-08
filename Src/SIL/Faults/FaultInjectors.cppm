/*
Filename: Src/SIL/Faults/FaultInjectors.cppm
Description: Fault injection family : value-semantic injector of declarative failure modes, timed activation and validated factory.
Exports:
    enum class InjectorError,
    class FaultInjector,
    make_fault_injector()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FaultInjectors;

import std;

import SilTypes;

export namespace sim::sil {

/*
Typed reasons why a declarative scenario cannot be turned into an injector
(infrastructure-domain configuration errors, never aircraft failures).
NoFault is a benign outcome (nominal scenario skipped by callers);
UnsupportedFailureMode marks documented failure modes without an injection
path yet; UnsupportedFaultTarget marks a target the current injection
machinery cannot disturb honestly (e.g. IMU/GNSS sensors or servo actuators).
*/
enum class InjectorError {
    NoFault,
    UnknownFailureMode,
    UnsupportedFailureMode,
    UnsupportedFaultTarget,
    InvalidLossProbability,
    InvalidSensorCorruption,
    InvalidEfficiency
};

/*
Value-semantic fault injector owning one declarative scenario. It replaces the
former polymorphic hierarchy so that no dynamic allocation is required.
*/
class FaultInjector {
public:
    FaultInjector() = default;
    explicit FaultInjector(const FaultScenario& scenario);

    /*
    Alters only the simulated environment (SimulationState) while the timed
    activation window of the owned scenario is open: the injector applies the
    physical representation of the failure mode (heartbeat silence, link
    cutoff, packet loss, sensor corruption, actuator efficiency loss). It
    never communicates with the HealthMonitor nor the SafetyManager: fault
    detection stays agnostic.
    */
    void inject(SimulationState& state, std::float64_t current_time) const;

    [[nodiscard]] bool is_active(std::float64_t current_time) const;

    [[nodiscard]] const FaultScenario& scenario() const noexcept;

private:
    FaultScenario scenario_{};
};

/*
Single dispatch point turning declarative fault scenarios into value
injectors: FailureMode::NONE yields InjectorError::NoFault (skipped by
callers), documented-but-unimplemented failure modes yield
UnsupportedFailureMode, targets without an honest injection path yield
UnsupportedFaultTarget and out-of-range scenario parameters yield typed
errors.
*/
[[nodiscard]] std::expected<FaultInjector, InjectorError> make_fault_injector(const FaultScenario& scenario);

}

namespace sim::sil {

void inject_fc1_unavailable(const FaultScenario& scenario, SimulationState& state);
void inject_communication_loss(const FaultScenario& scenario, SimulationState& state);
void inject_communication_degraded(const FaultScenario& scenario, SimulationState& state);
void inject_invalid_sensor_data(const FaultScenario& scenario, SimulationState& state);
void inject_actuator_degraded(const FaultScenario& scenario, SimulationState& state);

}
