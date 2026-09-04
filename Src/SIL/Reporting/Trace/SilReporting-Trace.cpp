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

/*
Writes the JSONL event trace of every scenario record: one independently
parseable JSON object per line, prefixed with the scenario name. The details
object of each event is composed by write_event_details().
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
