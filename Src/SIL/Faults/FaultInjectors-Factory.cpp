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

namespace {

/*
Honest target coverage of each failure mode: only the components the
injection machinery can actually disturb are accepted. The altitude channel
(barometer) is the only injectable sensor path and the main rotor is the only
injectable actuator; IMU/GNSS sensors and servo actuators are declared in the
FaultTarget vocabulary but have no injection path yet.
*/
bool target_supported(FailureMode mode, FaultTarget target)
{
    if (target == FaultTarget::Unspecified) {
        return true;
    }
    switch (mode) {
    case FailureMode::FC1_UNAVAILABLE:
        return target == FaultTarget::ProcessorFc1;
    case FailureMode::FC_COMMUNICATION_LOSS:
    case FailureMode::COMMUNICATION_DEGRADED:
        return target == FaultTarget::LinkFc1Fc2;
    case FailureMode::INVALID_SENSOR_DATA:
        return target == FaultTarget::SensorBarometer;
    case FailureMode::ACTUATOR_DEGRADED:
        return target == FaultTarget::ActuatorMainRotor;
    case FailureMode::NONE:
    case FailureMode::CONTROL_DEADLINE_MISSED:
    case FailureMode::INVALID_NUMERICAL_STATE:
        break;
    }
    return false;
}

}

/*
Validates the declarative scenario then wraps it into a value injector.
Nominal scenarios (NONE) produce InjectorError::NoFault so that callers can
skip them, documented failure modes without an injection path produce
UnsupportedFailureMode, targets outside the honest coverage produce
UnsupportedFaultTarget and out-of-range parameters produce the corresponding
typed error.
*/
std::expected<FaultInjector, InjectorError> make_fault_injector(const FaultScenario& scenario)
{
    if (scenario.failure_mode == FailureMode::NONE) {
        return std::unexpected(InjectorError::NoFault);
    }
    if (!failure_mode_implemented(scenario.failure_mode)) {
        return std::unexpected(InjectorError::UnsupportedFailureMode);
    }
    if (!target_supported(scenario.failure_mode, scenario.target)) {
        return std::unexpected(InjectorError::UnsupportedFaultTarget);
    }
    switch (scenario.failure_mode) {
    case FailureMode::FC1_UNAVAILABLE:
    case FailureMode::FC_COMMUNICATION_LOSS:
        return FaultInjector{scenario};
    case FailureMode::COMMUNICATION_DEGRADED:
        if (scenario.parameters.loss_probability < 0.0 || scenario.parameters.loss_probability > 1.0) {
            return std::unexpected(InjectorError::InvalidLossProbability);
        }
        return FaultInjector{scenario};
    case FailureMode::INVALID_SENSOR_DATA:
        if (scenario.parameters.corruption == SensorCorruptionMode::None) {
            return std::unexpected(InjectorError::InvalidSensorCorruption);
        }
        return FaultInjector{scenario};
    case FailureMode::ACTUATOR_DEGRADED:
        if (scenario.parameters.efficiency <= 0.0 || scenario.parameters.efficiency > 1.0) {
            return std::unexpected(InjectorError::InvalidEfficiency);
        }
        return FaultInjector{scenario};
    case FailureMode::NONE:
    case FailureMode::CONTROL_DEADLINE_MISSED:
    case FailureMode::INVALID_NUMERICAL_STATE:
        break;
    }
    return std::unexpected(InjectorError::UnknownFailureMode);
}

}
