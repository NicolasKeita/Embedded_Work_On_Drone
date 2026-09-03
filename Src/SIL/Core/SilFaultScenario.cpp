/*
Filename: Src/SIL/Core/SilFaultScenario.cpp
Description: Fault type naming helper for the declarative SIL scenarios.

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

}
