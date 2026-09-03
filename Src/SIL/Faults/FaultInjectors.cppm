/*
Filename: Src/SIL/Faults/FaultInjectors.cppm
Description: Fault injection family : value-semantic injector, timed activation and validated factory.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FaultInjectors;

import std;

import SilTypes;

export namespace sim::sil {

/*
Typed reasons why a declarative scenario cannot be turned into an injector.
NoFault is a benign outcome (nominal scenario skipped by callers), the other
values are real configuration errors.
*/
enum class InjectorError {
    NoFault,
    UnknownFaultType,
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
    activation window of the owned scenario is open. The injector never
    communicates with the HealthMonitor nor the SafetyManager: fault detection
    stays agnostic.
    */
    void inject(SimulationState& state, std::float64_t current_time) const;

    [[nodiscard]] bool is_active(std::float64_t current_time) const;

    [[nodiscard]] const FaultScenario& scenario() const noexcept;

private:
    FaultScenario scenario_{};
};

/*
Single dispatch point turning declarative fault scenarios into value injectors:
FaultType::None yields InjectorError::NoFault (skipped by callers), unknown
fault types and out-of-range scenario parameters yield typed errors.
*/
[[nodiscard]] std::expected<FaultInjector, InjectorError> make_fault_injector(const FaultScenario& scenario);

}

namespace sim::sil {

void inject_fc1_failure(const FaultScenario& scenario, SimulationState& state);
void inject_communication_fault(const FaultScenario& scenario, SimulationState& state);
void inject_sensor_fault(const FaultScenario& scenario, SimulationState& state);
void inject_actuator_fault(const FaultScenario& scenario, SimulationState& state);

}
