/*
Filename: Src/SIL/Runner/Events/Faults/SilRunnerEvents-Faults.cpp
Description: Fault injection chain recording and fault-cleared event emission.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilRunnerEvents;

import std;

import SilEvents;
import SilRunnerContext;
import SilTypes;

namespace sim::sil {

/*
Emits the fault injection chain rising edge: the generic injection marker
carrying the numeric parameters, the fault identity metadata and the expected
system response, followed by the typed sensor/actuator event when the fault
family has one.
*/
void record_fault_activation(RunContext& ctx, const FaultScenario& scenario)
{
    SilEvent injected{.timestamp = ctx.time,
                      .source = "ENV",
                      .type = SilEventType::FaultInjected,
                      .severity = EventSeverity::Info,
                      .detail = fault_type_name(scenario.fault_type),
                      .reason = fault_effect_reason(scenario)};

    write_fault_parameters(injected, scenario);
    write_fault_metadata(injected, scenario);
    if (scenario.duration > 0.0) {
        injected.duration_s = scenario.duration;
        injected.has_duration = true;
    }
    ctx.trace.record(injected);
    record_typed_fault(ctx, scenario, injected);
}

/*
Emits the fault-cleared marker when the activation window of a temporary fault
closes; permanent faults (duration <= 0) never produce this event. The cleared
marker repeats the target identity and carries the TEMPORARY profile.
*/
void record_fault_cleared(RunContext& ctx)
{
    SilEvent cleared{.timestamp = ctx.time,
                     .source = "ENV",
                     .type = SilEventType::FaultCleared,
                     .severity = EventSeverity::Info,
                     .detail = fault_type_name(ctx.last_fault_type),
                     .reason = "activation window closed",
                     .value = ctx.time - ctx.last_fault_start,
                     .has_value = true};

    cleared.target = fault_target_name(ctx.last_fault_target);
    cleared.target_signal = fault_target_signal(ctx.last_fault_target);
    cleared.profile = FaultProfile::Temporary;
    cleared.has_profile = true;
    ctx.trace.record(cleared);
}

}
