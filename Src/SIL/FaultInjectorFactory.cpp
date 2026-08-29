/*
Filename: Src/SIL/FaultInjectorFactory.cpp
Description: Scenario-to-injector dispatch implementation (single dispatch point).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectorFactory;

import std;

import IFaultInjector;
import FCFailureInjector;
import CommunicationFaultInjector;
import SensorFaultInjector;
import ActuatorFaultInjector;
import SilTypes;

namespace sim::sil {

std::unique_ptr<IFaultInjector> make_fault_injector(const FaultScenario& scenario)
{
    switch (scenario.fault_type) {
    case FaultType::None:
        return nullptr;
    case FaultType::FC1Failure:
        return std::make_unique<FCFailureInjector>(scenario);
    case FaultType::CommunicationLoss:
    case FaultType::CommunicationLossRate:
        return std::make_unique<CommunicationFaultInjector>(scenario);
    case FaultType::SensorFault:
        return std::make_unique<SensorFaultInjector>(scenario);
    case FaultType::ActuatorDegradation:
        return std::make_unique<ActuatorFaultInjector>(scenario);
    }
    return nullptr;
}

}
