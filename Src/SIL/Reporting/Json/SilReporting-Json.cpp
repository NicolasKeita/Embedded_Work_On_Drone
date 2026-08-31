/*
Filename: Src/SIL/Reporting/Json/SilReporting-Json.cpp
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

/* Writes the failure_reason field derived from the terminal mission state. */
void write_record_failure_reason(std::ostream& out, const SimulationResult& r)
{
    out << "    \"failure_reason\": ";
    if (r.final_state == sim::control::MissionState::ABORTED) {
        out << "\"Safe mode engage : ";
        write_json_escaped(out, fault_domain_name(r.first_fault_domain));
        out << "\"";
    }
    else {
        out << "\"\"";
    }
    out << ",\n";
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
