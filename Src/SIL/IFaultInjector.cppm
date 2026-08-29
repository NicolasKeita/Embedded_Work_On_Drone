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
    Altere uniquement l'environnement simule (SimulationState). L'injecteur ne
    communique jamais directement avec le HealthMonitor ni avec le
    SafetyManager : la detection des faultes reste agnostique.
    */
    virtual void inject(SimulationState& state, double current_time) = 0;

    [[nodiscard]] virtual bool is_active(double current_time) const = 0;

    [[nodiscard]] virtual FaultType fault_type() const = 0;
};

/*
Base commune aux injecteurs specialises : fenetre d'activation temporelle
[start_time, start_time + duration), une duration <= 0 signifiant actif
jusqu'a la fin de la simulation.
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
