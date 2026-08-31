/*
Filename: Src/SIL/Reporting/SilReporting-Json.cpp
Description: Streaming JSON writer for SIL simulation results export.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import FlightController;
import HealthMonitor;
import SafetyManager;
import SilTypes;

namespace sim::sil {

namespace {

/*
Writes one JSON record of the SIL results export, delegating the field groups
to the record writers.
*/
void write_record_json(std::ostream&         out,
                       const ScenarioRecord& record,
                       std::size_t           index,
                       std::size_t           record_count)
{
    out << "  {\n";
    write_record_mission(out, record);
    write_record_metrics(out, record, index + 1 == record_count);
}

}

/*
Writes the full JSON payload of the SIL results into the given stream.
*/
void write_json_payload(std::ostream& out, std::span<const ScenarioRecord> records)
{
    out << "[\n";
    for (std::size_t index = 0; index < records.size(); ++index) {
        write_record_json(out, records[index], index, records.size());
    }
    out << "]\n";
}

}
