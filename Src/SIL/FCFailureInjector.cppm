/*
Filename: Src/SIL/FCFailureInjector.cppm
Description: Fault injector interrupting FC1 heartbeat and status emission (crash / silence).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module FCFailureInjector;

import std;

import IFaultInjector;
import SilTypes;

export namespace sim::sil {

class FCFailureInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

}
