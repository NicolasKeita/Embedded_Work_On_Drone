/*
Filename: Src/SIL/Faults/FaultInjectors.cppm
Description: Fault injection family : common interface, timed base, concrete injectors and factory.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FaultInjectors;

import std;

import SilTypes;

export namespace sim::sil {

class IFaultInjector {
public:
    virtual ~IFaultInjector() = default;

    /*
    Alters only the simulated environment (SimulationState). The injector never
    communicates directly with the HealthMonitor nor the SafetyManager: fault
    detection stays agnostic.
    */
    virtual void inject(SimulationState& state, double current_time) = 0;

    [[nodiscard]] virtual bool is_active(double current_time) const = 0;

    [[nodiscard]] virtual FaultType fault_type() const = 0;
};

/*
Common base for specialized injectors: timed activation window
[start_time, start_time + duration), a duration <= 0 meaning active until the
end of the simulation.
*/
class TimedFaultInjector : public IFaultInjector {
public:
    explicit TimedFaultInjector(const FaultScenario& scenario);

    [[nodiscard]] bool is_active(double current_time) const override;

    [[nodiscard]] FaultType fault_type() const override;

protected:
    FaultScenario scenario_;
};

// Fault injector interrupting FC1 heartbeat and status emission (crash / silence).
class FCFailureInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

// Fault injector for total communication cutoff or random packet loss rate.
class CommunicationFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

// Fault injector corrupting sensor telemetry (out-of-range, NaN or extreme noise).
class SensorFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

// Fault injector altering actuator dynamics (efficiency loss, thrust reduction).
class ActuatorFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

/*
Single dispatch point turning declarative fault scenarios into polymorphic
injectors; returns nullptr for FaultType::None.
*/
[[nodiscard]] std::unique_ptr<IFaultInjector> make_fault_injector(const FaultScenario& scenario);

}
