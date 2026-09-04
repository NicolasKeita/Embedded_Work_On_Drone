/*
Filename: Src/SIL/Core/SilFaultScenario-Core.cpp
Description: Fault type naming and target resolution helpers of the declarative SIL scenarios.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilFaultScenario;

import std;

namespace sim::sil {

std::string_view fault_type_name(FaultType type)
{
    switch (type) {
    case FaultType::None:
        return "NOMINAL";
    case FaultType::FC1Failure:
        return "FC1_FAILURE";
    case FaultType::CommunicationLoss:
        return "COMMUNICATION_LOSS";
    case FaultType::CommunicationLossRate:
        return "COMMUNICATION_LOSS_RATE";
    case FaultType::SensorFault:
        return "SENSOR_FAULT";
    case FaultType::ActuatorDegradation:
        return "ACTUATOR_DEGRADATION";
    }
    return "UNKNOWN";
}

/*
Target actually disturbed by the scenario: the explicit target when the
scenario names one, otherwise the canonical component altered by the fault
family (FC1 processor, FC1-FC2 link, altitude channel or main-rotor actuator).
*/
FaultTarget effective_fault_target(const FaultScenario& scenario)
{
    if (scenario.target != FaultTarget::Unspecified) {
        return scenario.target;
    }
    switch (scenario.fault_type) {
    case FaultType::FC1Failure:
        return FaultTarget::ProcessorFc1;
    case FaultType::CommunicationLoss:
    case FaultType::CommunicationLossRate:
        return FaultTarget::LinkFc1Fc2;
    case FaultType::SensorFault:
        return FaultTarget::SensorBarometer;
    case FaultType::ActuatorDegradation:
        return FaultTarget::ActuatorMainRotor;
    case FaultType::None:
        break;
    }
    return FaultTarget::Unspecified;
}

}
