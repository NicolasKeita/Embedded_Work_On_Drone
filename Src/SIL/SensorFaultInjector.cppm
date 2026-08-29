/*
Filename: Src/SIL/SensorFaultInjector.cppm
Description: Fault injector corrupting sensor telemetry (out-of-range, NaN or extreme noise).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SensorFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

export namespace sim::sil {

class SensorFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

}
