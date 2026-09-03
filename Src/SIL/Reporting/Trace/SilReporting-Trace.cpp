/*
Filename: Src/SIL/Reporting/Trace/SilReporting-Trace.cpp
Description: JSONL event trace writer for the structured SIL event artifacts.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import SilEvents;
import SilTypes;

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
}

/*
Writes the details object of one JSONL event: only the set fields are emitted.
*/
void write_event_details(std::ostream& out, const SilEvent& event)
{
    if (event.detail.empty() && event.previous_state.empty() && event.new_state.empty() && event.reason.empty()
        && !event.has_sequence && !event.has_latency && !event.has_value) {
        return;
    }
    out << ",\"details\":{";
    bool first = true;

    write_string_field(out, "detail", event.detail, first);
    write_string_field(out, "previous_state", event.previous_state, first);
    write_string_field(out, "new_state", event.new_state, first);
    write_string_field(out, "reason", event.reason, first);
    write_event_number_details(out, event, first);
    out << "}";
}

}

/*
Writes the JSONL event trace of every scenario record: one independently
parseable JSON object per line, prefixed with the scenario name.
*/
void write_jsonl_trace(std::ostream& out, std::span<const ScenarioRecord> records)
{
    for (const ScenarioRecord& record : records) {
        for (const SilEvent& event : record.events) {
            out << "{\"scenario\":\"";
            write_json_escaped(out, record.name);
            out << "\",\"timestamp\":";
            write_seconds(out, event.timestamp);
            out << ",\"source\":\"";
            write_json_escaped(out, event.source);
            out << "\",\"event\":\"" << event_type_name(event.type) << "\",\"severity\":\""
                << event_severity_name(event.severity) << "\"";
            write_event_details(out, event);
            out << "}\n";
        }
    }
}

}
