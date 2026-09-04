/*
Filename: Src/SIL/Reporting/Trace/SilReporting-Trace-Text.cpp
Description: Human-readable text trace writer with structured fault-injection blocks (debugging artifact).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import SilEvents;
import SilTypes;

namespace sim::sil {

namespace {

constexpr std::string_view kFaultBranch = "  \u251C\u2500\u2500 ";
constexpr std::string_view kFaultLast = "  \u2514\u2500\u2500 ";

/*
Streams the details of one text trace line (states, reason, sequence, latency,
value) exactly when the fields are set.
*/
void write_event_text_details(std::ostream& out, const SilEvent& event)
{
    if (!event.previous_state.empty()) {
        out << ' ' << event.previous_state << " -> " << event.new_state;
    }
    if (!event.detail.empty()) {
        out << ' ' << event.detail;
    }
    if (!event.reason.empty()) {
        out << " reason=" << event.reason;
    }
    if (event.has_sequence) {
        out << " seq=" << event.sequence;
    }
    if (event.has_latency) {
        out << " latency=";
        write_metric(out, 1000.0 * event.latency_s);
        out << "ms";
    }
    if (event.has_value) {
        out << " value=";
        write_metric(out, event.value);
    }
    if (event.has_duration) {
        out << " duration=";
        write_seconds(out, event.duration_s);
        out << "s";
    }
}

/*
Streams the structured multi-line block of one fault-injection event: type,
target, temporality, duration, parameters and expected system response.
*/
void write_fault_block(std::ostream& out, const SilEvent& event)
{
    out << '[';
    write_seconds(out, event.timestamp);
    out << "] [FAULT] [" << event_type_name(event.type) << "]\n";
    out << kFaultBranch << "Type : " << event.detail;
    if (!event.subtype.empty()) {
        out << " (" << event.subtype << ')';
    }
    out << '\n';
    out << kFaultBranch << "Target : " << event.target;
    if (!event.target_signal.empty()) {
        out << " (" << event.target_signal << ')';
    }
    out << '\n';
    out << kFaultBranch << "Profile : " << (event.has_profile ? fault_profile_name(event.profile) : "UNSPECIFIED")
        << '\n';
    out << kFaultBranch << "Duration : ";
    if (event.has_duration) {
        write_metric(out, event.duration_s);
        out << "s (ends at t=";
        write_seconds(out, event.timestamp + event.duration_s);
        out << "s)";
    }
    else {
        out << "N/A";
    }
    out << '\n';
    if (event.value_kind != FaultValueKind::None) {
        out << kFaultBranch << "Parameters : ";
        write_fault_value(out, event);
        out << '\n';
    }
    out << kFaultLast << "Expected Behavior : " << event.expected_behavior << '\n';
}

}

/*
Writes one human-readable text entry per event: fault injections use the
structured multi-line block, every other event stays on one line.
*/
void write_text_trace(std::ostream& out, std::span<const SilEvent> events)
{
    for (const SilEvent& event : events) {
        if (event.type == SilEventType::FaultInjected) {
            write_fault_block(out, event);
            continue;
        }
        write_seconds(out, event.timestamp);
        out << ' ' << event.source << ' ' << event_type_name(event.type);
        write_event_text_details(out, event);
        out << '\n';
    }
}

}
