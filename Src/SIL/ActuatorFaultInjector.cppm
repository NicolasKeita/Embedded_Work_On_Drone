/*
Filename: Src/SIL/ActuatorFaultInjector.cppm
Description: Fault injector altering actuator dynamics (efficiency loss, thrust reduction).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module ActuatorFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

export namespace sim::sil {

class ActuatorFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

}
