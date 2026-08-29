/*
Filename: Src/SIL/IFaultInjector.cppm
Description: Abstract fault injector interface and shared timed activation window base.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module IFaultInjector;

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

}
