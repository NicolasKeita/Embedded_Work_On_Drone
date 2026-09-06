/*
Filename: Src/SIL/Reporting/Trace/SilReporting-Trace-Details.cpp
Description: Details-object composer of the JSONL event trace: identity, temporality and numeric fields.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import SilEvents;
import SilFaultScenario;

namespace sim::sil {

namespace {

/*
Writes one string-valued details field when the value is not empty.
*/
void write_string_field(std::ostream& out, std::string_view name, std::string_view value, bool& first)
{
    if (value.empty()) {
        return;
    }
    if (!first) {
        out << ",";
    }
    first = false;
    out << "\"" << name << "\":\"";
    write_json_escaped(out, value);
    out << "\"";
}

/*
Writes the sequence, latency and value fields of the details object.
*/
void write_event_number_details(std::ostream& out, const SilEvent& event, bool& first)
{
    if (event.has_sequence) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"sequence\":" << event.sequence;
    }
    if (event.has_latency) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"latency_s\":";
        write_seconds(out, event.latency_s);
    }
    if (event.has_value) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"value\":";
        write_metric(out, event.value);
    }
    if (event.has_duration) {
        if (!first) {
            out << ",";
        }
        first = false;
        out << "\"duration_s\":";
        write_seconds(out, event.duration_s);
    }
}

}

/*
Writes the details object of one JSONL event: only the set fields are emitted.
Fault injections carry the target identity, the corruption subtype, the
temporality profile, the labelled value kind and the expected system response.
*/
void write_event_details(std::ostream& out, const SilEvent& event)
{
    if (event.detail.empty() && event.subtype.empty() && event.target.empty() && event.target_signal.empty()
        && event.physical_role.empty() && event.physical_category.empty() && event.physical_function.empty()
        && event.previous_state.empty() && event.new_state.empty() && event.reason.empty()
        && event.expected_behavior.empty() && !event.has_profile && event.value_kind == FaultValueKind::None
        && !event.has_sequence && !event.has_latency && !event.has_value && !event.has_duration) {
        return;
    }
    out << ",\"details\":{";
    bool first = true;

    write_string_field(out, "detail", event.detail, first);
    write_string_field(out, "subtype", event.subtype, first);
    write_string_field(out, "target", event.target, first);
    write_string_field(out, "target_signal", event.target_signal, first);
    write_string_field(out, "physical_role", event.physical_role, first);
    write_string_field(out, "physical_category", event.physical_category, first);
    write_string_field(out, "physical_function", event.physical_function, first);
    write_string_field(out, "previous_state", event.previous_state, first);
    write_string_field(out, "new_state", event.new_state, first);
    write_string_field(out, "reason", event.reason, first);
    write_string_field(out, "expected_behavior", event.expected_behavior, first);
    if (event.has_profile) {
        write_string_field(out, "profile", fault_profile_name(event.profile), first);
    }
    if (event.value_kind != FaultValueKind::None) {
        write_string_field(out, "value_kind", fault_value_kind_name(event.value_kind), first);
    }
    write_event_number_details(out, event, first);
    out << "}";
}

}
