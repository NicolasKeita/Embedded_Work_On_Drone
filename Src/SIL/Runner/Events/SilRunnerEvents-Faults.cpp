/*
Filename: Src/SIL/Runner/Events/SilRunnerEvents-Faults.cpp
Description: Fault injection chain recording and fault-cleared event emission.

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
Enriches the injection marker with the numeric scenario parameters: loss
probability, actuator efficiency or forced corrupted altitude.
*/

static void write_fault_parameters(SilEvent& event, const FaultScenario& scenario)
{
    if (scenario.fault_type == FaultType::CommunicationLossRate) {
        event.value = scenario.parameters.loss_probability;
        event.has_value = true;
    }
    if (scenario.fault_type == FaultType::ActuatorDegradation) {
        event.value = scenario.parameters.efficiency;
        event.has_value = true;
    }
    if (scenario.fault_type == FaultType::SensorFault
        && scenario.parameters.corruption == SensorCorruptionMode::AltitudeOutOfRange) {
        event.value = scenario.parameters.corrupted_altitude_m;
        event.has_value = true;
    }
}

/*
Emits the fault injection chain: the generic injection marker carrying the
fault parameters plus the typed sensor/actuator events. Called on a rising edge.
*/
void record_fault_activation(RunContext& ctx, const FaultScenario& scenario)
{
    SilEvent injected;

    injected.timestamp = ctx.time;
    injected.source = "ENV";
    injected.type = SilEventType::FaultInjected;
    injected.severity = EventSeverity::Info;
    injected.detail = fault_type_name(scenario.fault_type);
    injected.reason = fault_effect_reason(scenario);
    write_fault_parameters(injected, scenario);
    ctx.trace.record(injected);

    const auto record_typed = [&ctx, &scenario](SilEventType type, std::float64_t value) {
        SilEvent event;

        event.timestamp = ctx.time;
        event.source = "ENV";
        event.type = type;
        event.severity = EventSeverity::Warning;
        event.value = value;
        event.has_value = true;
        ctx.trace.record(event);
    };

    if (scenario.fault_type == FaultType::SensorFault) {
        record_typed(SilEventType::SensorFault, scenario.parameters.corrupted_altitude_m);
    }
    if (scenario.fault_type == FaultType::ActuatorDegradation) {
        record_typed(SilEventType::ActuatorFault, scenario.parameters.efficiency);
    }
}

/*
Emits the fault-cleared marker when the activation window of a temporary fault
closes; permanent faults (duration <= 0) never produce this event.
*/
void record_fault_cleared(RunContext& ctx)
{
    SilEvent cleared;

    cleared.timestamp = ctx.time;
    cleared.source = "ENV";
    cleared.type = SilEventType::FaultCleared;
    cleared.severity = EventSeverity::Info;
    cleared.detail = fault_type_name(ctx.last_fault_type);
    cleared.reason = "activation window closed";
    cleared.value = ctx.time - ctx.last_fault_start;
    cleared.has_value = true;
    ctx.trace.record(cleared);
}

}
