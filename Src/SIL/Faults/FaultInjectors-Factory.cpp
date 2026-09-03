/*
Filename: Src/SIL/Faults/FaultInjectors-Factory.cpp
Description: Validated scenario-to-injector dispatch (single dispatch point).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module FaultInjectors;

import std;

import SilTypes;
import Telemetry;

namespace sim::sil {

/*
Validates the declarative scenario then wraps it into a value injector. Nominal
scenarios (None) produce InjectorError::NoFault so that callers can skip them,
and out-of-range parameters produce the corresponding typed error.
*/
std::expected<FaultInjector, InjectorError> make_fault_injector(const FaultScenario& scenario)
{
    switch (scenario.fault_type) {
    case FaultType::None:
        return std::unexpected(InjectorError::NoFault);
    case FaultType::FC1Failure:
    case FaultType::CommunicationLoss:
        return FaultInjector{scenario};
    case FaultType::CommunicationLossRate:
        if (scenario.parameters.loss_probability < 0.0 || scenario.parameters.loss_probability > 1.0) {
            return std::unexpected(InjectorError::InvalidLossProbability);
        }
        return FaultInjector{scenario};
    case FaultType::SensorFault:
        if (scenario.parameters.corruption == SensorCorruptionMode::None) {
            return std::unexpected(InjectorError::InvalidSensorCorruption);
        }
        return FaultInjector{scenario};
    case FaultType::ActuatorDegradation:
        if (scenario.parameters.efficiency <= 0.0 || scenario.parameters.efficiency > 1.0) {
            return std::unexpected(InjectorError::InvalidEfficiency);
        }
        return FaultInjector{scenario};
    }
    return std::unexpected(InjectorError::UnknownFaultType);
}

}
