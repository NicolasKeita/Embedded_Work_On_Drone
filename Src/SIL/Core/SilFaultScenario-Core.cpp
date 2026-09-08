/*
Filename: Src/SIL/Core/SilFaultScenario-Core.cpp
Description: Fault domain / failure mode naming and target resolution helpers of the declarative SIL scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilFaultScenario;

import std;

namespace sim::sil {

std::string_view fault_domain_name(FaultDomain domain)
{
    switch (domain) {
    case FaultDomain::SYSTEM:
        return "SYSTEM";
    case FaultDomain::HIL:
        return "HIL";
    case FaultDomain::INFRASTRUCTURE:
        return "INFRASTRUCTURE";
    }
    return "UNKNOWN";
}

std::string_view failure_mode_name(FailureMode mode)
{
    switch (mode) {
    case FailureMode::NONE:
        return "NOMINAL";
    case FailureMode::FC1_UNAVAILABLE:
        return "FC1_UNAVAILABLE";
    case FailureMode::FC_COMMUNICATION_LOSS:
        return "FC_COMMUNICATION_LOSS";
    case FailureMode::COMMUNICATION_DEGRADED:
        return "COMMUNICATION_DEGRADED";
    case FailureMode::INVALID_SENSOR_DATA:
        return "INVALID_SENSOR_DATA";
    case FailureMode::ACTUATOR_DEGRADED:
        return "ACTUATOR_DEGRADED";
    case FailureMode::CONTROL_DEADLINE_MISSED:
        return "CONTROL_DEADLINE_MISSED";
    case FailureMode::INVALID_NUMERICAL_STATE:
        return "INVALID_NUMERICAL_STATE";
    }
    return "UNKNOWN";
}

/*
Target actually disturbed by the scenario: the explicit target when the
scenario names one, otherwise the canonical component altered by the failure
mode (FC1 processor, FC1-FC2 link, altitude channel or main-rotor actuator).
Documented failure modes without an injection path resolve to Unspecified.
*/
FaultTarget effective_fault_target(const FaultScenario& scenario)
{
    if (scenario.target != FaultTarget::Unspecified) {
        return scenario.target;
    }
    switch (scenario.failure_mode) {
    case FailureMode::FC1_UNAVAILABLE:
        return FaultTarget::ProcessorFc1;
    case FailureMode::FC_COMMUNICATION_LOSS:
    case FailureMode::COMMUNICATION_DEGRADED:
        return FaultTarget::LinkFc1Fc2;
    case FailureMode::INVALID_SENSOR_DATA:
        return FaultTarget::SensorBarometer;
    case FailureMode::ACTUATOR_DEGRADED:
        return FaultTarget::ActuatorMainRotor;
    case FailureMode::NONE:
    case FailureMode::CONTROL_DEADLINE_MISSED:
    case FailureMode::INVALID_NUMERICAL_STATE:
        break;
    }
    return FaultTarget::Unspecified;
}

}
