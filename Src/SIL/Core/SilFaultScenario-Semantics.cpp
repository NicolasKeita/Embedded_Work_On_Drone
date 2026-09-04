/*
Filename: Src/SIL/Core/SilFaultScenario-Semantics.cpp
Description: Actuator semantic mapping : physical role, hardware category and aerodynamic function of fault targets.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilFaultScenario;

import std;

namespace sim::sil {

/*
Physical role of a fault target: the functional name of the affected component,
resolved from the aircraft model configuration. The current single-rotor VTOL
airframe maps the main rotor to propulsion and the two servos to pitch/roll
control; multirotor and fixed-wing airframes would resolve their own actuator
identifiers here. Empty for non-actuator targets.
*/
std::string_view fault_target_physical_role(FaultTarget target)
{
    switch (target) {
    case FaultTarget::ActuatorMainRotor:
        return "main_rotor";
    case FaultTarget::ActuatorLeftServo:
        return "left_servo";
    case FaultTarget::ActuatorRightServo:
        return "right_servo";
    case FaultTarget::Unspecified:
    case FaultTarget::SensorBarometer:
    case FaultTarget::SensorImu:
    case FaultTarget::SensorGnss:
    case FaultTarget::SensorRpmFeedback:
    case FaultTarget::ProcessorFc1:
    case FaultTarget::LinkFc1Fc2:
        break;
    }
    return {};
}

/*
Physical category of a fault target: the hardware family of the affected
component. Rotor motors provide thrust, control-surface servos drive
aerodynamic deflections and thrust-vector units redirect propulsion. Empty
for non-actuator targets.
*/
std::string_view fault_target_category(FaultTarget target)
{
    switch (target) {
    case FaultTarget::ActuatorMainRotor:
        return "ROTOR_MOTOR";
    case FaultTarget::ActuatorLeftServo:
    case FaultTarget::ActuatorRightServo:
        return "CONTROL_SURFACE_SERVO";
    case FaultTarget::Unspecified:
    case FaultTarget::SensorBarometer:
    case FaultTarget::SensorImu:
    case FaultTarget::SensorGnss:
    case FaultTarget::SensorRpmFeedback:
    case FaultTarget::ProcessorFc1:
    case FaultTarget::LinkFc1Fc2:
        break;
    }
    return {};
}

/*
Aerodynamic or propulsion function served by a fault target. The main rotor
provides thrust and attitude control on a VTOL, the paired servos drive the
pitch (mean) and roll (differential) channels. Empty for non-actuator targets.
*/
std::string_view fault_target_function(FaultTarget target)
{
    switch (target) {
    case FaultTarget::ActuatorMainRotor:
        return "Propulsion / Roll-Pitch-Yaw Control";
    case FaultTarget::ActuatorLeftServo:
    case FaultTarget::ActuatorRightServo:
        return "Pitch / Roll Control";
    case FaultTarget::Unspecified:
    case FaultTarget::SensorBarometer:
    case FaultTarget::SensorImu:
    case FaultTarget::SensorGnss:
    case FaultTarget::SensorRpmFeedback:
    case FaultTarget::ProcessorFc1:
    case FaultTarget::LinkFc1Fc2:
        break;
    }
    return {};
}

}
