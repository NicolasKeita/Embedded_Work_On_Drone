/*
Filename: Src/SIL/CommunicationFaultInjector.cppm
Description: Fault injector for total communication cutoff or random packet loss rate.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module CommunicationFaultInjector;

import std;

import IFaultInjector;
import SilTypes;

export namespace sim::sil {

class CommunicationFaultInjector final : public TimedFaultInjector {
public:
    using TimedFaultInjector::TimedFaultInjector;

    void inject(SimulationState& state, double current_time) override;
};

}
