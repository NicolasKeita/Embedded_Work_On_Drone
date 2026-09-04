/*
Filename: Src/SIL/Core/SilFaultScenario-Names.cpp
Description: Human-readable names of the fault vocabulary : targets, signals, temporality profiles and value kinds.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilFaultScenario;

import std;

namespace sim::sil {

/* Canonical identifier of a fault target for the Target field of the injection logs. */
std::string_view fault_target_name(FaultTarget target)
{
    switch (target) {
    case FaultTarget::Unspecified:
        return "unspecified";
    case FaultTarget::ActuatorMainRotor:
        return "actuator_0";
    case FaultTarget::ActuatorLeftServo:
        return "actuator_1";
    case FaultTarget::ActuatorRightServo:
        return "actuator_2";
    case FaultTarget::SensorBarometer:
        return "baro_primary";
    case FaultTarget::SensorImu:
        return "imu_0";
    case FaultTarget::SensorGnss:
        return "gnss_primary";
    case FaultTarget::SensorRpmFeedback:
        return "rpm_feedback_0";
    case FaultTarget::ProcessorFc1:
        return "fc1_primary";
    case FaultTarget::LinkFc1Fc2:
        return "comms_link_fc1_fc2";
    }
    return "unknown";
}

/*
Simulated signal path disturbed on the target, shown next to the target
identifier (main-rotor command, servo command, attitude or link state).
*/
std::string_view fault_target_signal(FaultTarget target)
{
    switch (target) {
    case FaultTarget::ActuatorMainRotor:
        return "wing_rpm";
    case FaultTarget::ActuatorLeftServo:
        return "left_servo_angle";
    case FaultTarget::ActuatorRightServo:
        return "right_servo_angle";
    case FaultTarget::SensorImu:
        return "pitch_roll";
    case FaultTarget::SensorGnss:
        return "position_xy";
    case FaultTarget::SensorRpmFeedback:
        return "actual_rpm";
    case FaultTarget::ProcessorFc1:
        return "heartbeat";
    case FaultTarget::LinkFc1Fc2:
        return "link_state";
    case FaultTarget::Unspecified:
    case FaultTarget::SensorBarometer:
        break;
    }
    return {};
}

/*
Human-readable temporality profile of a fault activation window, as displayed
in the Profile field of the fault injection logs.
*/
std::string_view fault_profile_name(FaultProfile profile)
{
    switch (profile) {
    case FaultProfile::Permanent:
        return "PERMANENT";
    case FaultProfile::Temporary:
        return "TEMPORARY";
    }
    return "UNKNOWN";
}

/*
Human-readable semantic of the numeric parameter of a fault event, used by the
JSON trace to label the raw value.
*/
std::string_view fault_value_kind_name(FaultValueKind kind)
{
    switch (kind) {
    case FaultValueKind::Efficiency:
        return "efficiency";
    case FaultValueKind::LossProbability:
        return "loss_probability";
    case FaultValueKind::Altitude:
        return "altitude";
    case FaultValueKind::AltitudeNoise:
        return "altitude_noise";
    case FaultValueKind::None:
        break;
    }
    return {};
}

}
