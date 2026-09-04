/*
Filename: Src/SIL/Reporting/SilReporting.cppm
Description: SIL validation artifacts export: JSON, CSV payload and trace writers plus orchestration of the report sub-module.
Exports:
    write_json_escaped(),
    write_json_payload(),
    write_csv_payload(),
    write_jsonl_trace(),
    write_text_trace(),
    write_text_trace_report(),
    write_sil_report()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReporting;

import std;

import SilEvents;

export import SilReportingReport;

export namespace sim::sil {

void write_json_escaped(std::ostream& out, std::string_view text);
void write_json_payload(std::ostream& out, std::span<const ScenarioRecord> records);
void write_csv_payload(std::ostream& out, std::span<const ScenarioRecord> records);

/*
Writes the machine-readable JSONL event trace of every scenario record (one
independently parseable JSON object per line).
*/
void write_jsonl_trace(std::ostream& out, std::span<const ScenarioRecord> records);

/*
Writes one human-readable text trace line per event (debugging artifact).
*/
void write_text_trace(std::ostream& out, std::span<const SilEvent> events);

/*
Writes the human-readable text trace of every scenario record (one section per
scenario), used for the sil_trace.txt artifact.
*/
void write_text_trace_report(std::ostream& out, std::span<const ScenarioRecord> records);

/*
Generates the SIL validation artifacts (docs/validation/data/sil.md, sil.json,
sil.csv, sil_trace.jsonl); returns a typed error when disk writing fails.
*/
[[nodiscard]] std::expected<void, ReportError> write_sil_report(std::span<const ScenarioRecord> records,
                                                                const SilReportOptions& options = {});

}

namespace sim::sil {

// Streams one report section into a freshly opened file, or returns a typed error.
std::expected<void, ReportError> write_section_file(const std::filesystem::path& path,
                                                    ReportSectionWriter          writer,
                                                    std::span<const ScenarioRecord> records);

// Writes the identity and mission fields of one JSON record.
void write_record_mission(std::ostream& out, const ScenarioRecord& record);

// Writes the timing, metric and verdict fields of one JSON record, closing the object.
void write_record_metrics(std::ostream& out, const ScenarioRecord& record, bool last);

// Writes the details object of one JSONL event (only the set fields are emitted).
void write_event_details(std::ostream& out, const SilEvent& event);

// Streams the inline details of one text trace line (states, reason, value).
void write_event_text_details(std::ostream& out, const SilEvent& event);

}
