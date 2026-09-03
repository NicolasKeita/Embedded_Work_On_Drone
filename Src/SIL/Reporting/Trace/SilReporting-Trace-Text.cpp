/*
Filename: Src/SIL/Reporting/Trace/SilReporting-Trace-Text.cpp
Description: Human-readable text event trace writer (debugging artifact).

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import SilEvents;

namespace sim::sil {

namespace {

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
}

}

/*
Writes one human-readable text trace line per event.
*/
void write_text_trace(std::ostream& out, std::span<const SilEvent> events)
{
    for (const SilEvent& event : events) {
        write_seconds(out, event.timestamp);
        out << ' ' << event.source << ' ' << event_type_name(event.type);
        write_event_text_details(out, event);
        out << '\n';
    }
}

}
