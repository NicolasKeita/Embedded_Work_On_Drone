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
Semantic of the numeric payload of a sensor fault: forced altitude for the
out-of-range mode and noise amplitude for the extreme-noise mode; the NaN mode
carries no usable numeric value.
*/
static FaultValueKind sensor_fault_value_kind(SensorCorruptionMode corruption)
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
    switch (scenario.fault_type) {
    case FaultType::CommunicationLossRate:
        event.value = scenario.parameters.loss_probability;
        event.has_value = true;
        event.value_kind = FaultValueKind::LossProbability;
        break;
    case FaultType::ActuatorDegradation:
        event.value = scenario.parameters.efficiency;
        event.has_value = true;
        event.value_kind = FaultValueKind::Efficiency;
        break;
    case FaultType::SensorFault:
        event.value_kind = sensor_fault_value_kind(scenario.parameters.corruption);
        if (scenario.parameters.corruption == SensorCorruptionMode::AltitudeOutOfRange) {
            event.value = scenario.parameters.corrupted_altitude_m;
            event.has_value = true;
        }
        if (scenario.parameters.corruption == SensorCorruptionMode::ExtremeNoise) {
            event.value = kExtremeNoiseAmplitudeM;
            event.has_value = true;
        }
        break;
    case FaultType::None:
    case FaultType::FC1Failure:
    case FaultType::CommunicationLoss:
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
    if (scenario.fault_type == FaultType::SensorFault) {
        event.subtype = corruption_mode_name(scenario.parameters.corruption);
    }
    event.profile = scenario.duration > 0.0 ? FaultProfile::Temporary : FaultProfile::Permanent;
    event.has_profile = true;
    event.expected_behavior = fault_expected_behavior(scenario);
}

/*
Emits the typed sensor/actuator fault event echoing the injection marker
payload; fault families without a typed event emit nothing.
*/
void record_typed_fault(RunContext& ctx, const FaultScenario& scenario, const SilEvent& injected)
{
    if (scenario.fault_type != FaultType::SensorFault && scenario.fault_type != FaultType::ActuatorDegradation) {
        return;
    }
    const SilEventType type = scenario.fault_type == FaultType::SensorFault ? SilEventType::SensorFault
                                                                           : SilEventType::ActuatorFault;
    SilEvent event{.timestamp = ctx.time,
                   .source = "ENV",
                   .type = type,
                   .severity = EventSeverity::Warning,
                   .value = injected.value,
                   .has_value = injected.has_value};

    event.target = injected.target;
    event.value_kind = injected.value_kind;
    ctx.trace.record(event);
}

}