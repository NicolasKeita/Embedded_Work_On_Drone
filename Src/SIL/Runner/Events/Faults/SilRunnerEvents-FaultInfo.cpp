/*
Filename: Src/SIL/Runner/Events/Faults/SilRunnerEvents-FaultInfo.cpp
Description: Fault event field composition : numeric parameters, target metadata and typed fault emission.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import SilEvents;
import SilRunnerContext;
import SilTypes;
import Telemetry;

namespace sim::sil {

/*
Semantic of the numeric payload of an invalid sensor data failure mode: forced
altitude for the out-of-range mode and noise amplitude for the extreme-noise
mode; the NaN mode carries no usable numeric value.
*/
static FaultValueKind invalid_sensor_data_value_kind(SensorCorruptionMode corruption)
{
    switch (corruption) {
    case SensorCorruptionMode::AltitudeOutOfRange:
    case SensorCorruptionMode::AltitudeNaN:
        return FaultValueKind::Altitude;
    case SensorCorruptionMode::ExtremeNoise:
        return FaultValueKind::AltitudeNoise;
    case SensorCorruptionMode::None:
        break;
    }
    return FaultValueKind::None;
}

/*
Enriches the injection marker with the numeric scenario parameters: loss
probability, actuator efficiency, forced corrupted altitude or noise amplitude,
each labelled by its value kind for the report renderers.
*/
void write_fault_parameters(SilEvent& event, const FaultScenario& scenario)
{
    switch (scenario.failure_mode) {
    case FailureMode::COMMUNICATION_DEGRADED:
        event.value = scenario.parameters.loss_probability;
        event.has_value = true;
        event.value_kind = FaultValueKind::LossProbability;
        break;
    case FailureMode::ACTUATOR_DEGRADED:
        event.value = scenario.parameters.efficiency;
        event.has_value = true;
        event.value_kind = FaultValueKind::Efficiency;
        break;
    case FailureMode::INVALID_SENSOR_DATA:
        event.value_kind = invalid_sensor_data_value_kind(scenario.parameters.corruption);
        if (scenario.parameters.corruption == SensorCorruptionMode::AltitudeOutOfRange) {
            event.value = scenario.parameters.corrupted_altitude_m;
            event.has_value = true;
        }
        if (scenario.parameters.corruption == SensorCorruptionMode::ExtremeNoise) {
            event.value = kExtremeNoiseAmplitudeM;
            event.has_value = true;
        }
        break;
    case FailureMode::NONE:
    case FailureMode::FC1_UNAVAILABLE:
    case FailureMode::FC_COMMUNICATION_LOSS:
    case FailureMode::CONTROL_DEADLINE_MISSED:
    case FailureMode::INVALID_NUMERICAL_STATE:
        break;
    }
}

/*
Completes the injection marker with the fault identity metadata: canonical
target and disturbed signal, corruption subtype, temporality profile and the
nominal response expected from the supervised system.
*/
void write_fault_metadata(SilEvent& event, const FaultScenario& scenario)
{
    const FaultTarget target = effective_fault_target(scenario);

    event.target = fault_target_name(target);
    event.target_signal = fault_target_signal(target);
    event.physical_role = fault_target_physical_role(target);
    event.physical_category = fault_target_category(target);
    event.physical_function = fault_target_function(target);
    if (scenario.failure_mode == FailureMode::INVALID_SENSOR_DATA) {
        event.subtype = corruption_mode_name(scenario.parameters.corruption);
    }
    event.profile = scenario.duration > 0.0 ? FaultProfile::Temporary : FaultProfile::Permanent;
    event.has_profile = true;
    event.expected_behavior = fault_expected_behavior(scenario);
}

}
