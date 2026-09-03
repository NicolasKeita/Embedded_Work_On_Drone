/*
Filename: Src/SIL/Reporting/SilReporting.cppm
Description: SIL validation artifacts export : Markdown, JSON and CSV writers plus shared formatting helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReporting;

import std;

import SilEvents;
import SilTelemetry;
import SilTypes;

export namespace sim::sil {

// Typed report generation failures (no exception is ever thrown).
enum class ReportError { DirectoryCreation, FileOpen };

struct ScenarioRecord {
    std::string_view                 name;
    FaultScenario                    scenario;
    SimulationResult                 result;
    std::span<const SilEvent>        events{};
    std::span<const TelemetrySample> telemetry{};
    std::span<const TrueStateSample> ground_truth{};
};

// Writes one full report section into a stream (fixed signature for file writers).
using ReportSectionWriter = void (*)(std::ostream&, std::span<const ScenarioRecord>);

struct SilReportOptions {
    std::filesystem::path docs_dir = "docs/validation";
    bool                  write_markdown = true;
    bool                  write_json = true;
    bool                  write_csv = true;
    bool                  write_trace = true;
    bool                  write_telemetry = true;
    std::float64_t        telemetry_report_interval_s = 2.5;
};

[[nodiscard]] std::string_view yes_no(bool value) noexcept;
void write_seconds(std::ostream& out, std::float64_t value);
void write_metric(std::ostream& out, std::float64_t value);
void write_json_escaped(std::ostream& out, std::string_view text);
void write_csv_escaped(std::ostream& out, std::string_view text);
void write_markdown_report(std::ostream& out, std::span<const ScenarioRecord> records,
                           std::float64_t telemetry_report_interval_s = 2.5);
void write_json_payload(std::ostream& out, std::span<const ScenarioRecord> records);
void write_csv_payload(std::ostream& out, std::span<const ScenarioRecord> records);

/*
Writes the raw structured sensor telemetry of every scenario record into one
CSV stream (one row per sample, scenario column included).
*/
void write_telemetry_csv(std::ostream& out, std::span<const ScenarioRecord> records);

/*
Writes the raw physics ground truth of every scenario record into one CSV
stream, kept separate from the sensor telemetry.
*/
void write_truth_csv(std::ostream& out, std::span<const ScenarioRecord> records);

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
Generates the SIL validation artifacts (docs/validation/sil.md, sil.json,
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

// Writes the periodic telemetry table of one scenario section.
void write_telemetry_table(std::ostream& out, std::span<const TelemetrySample> samples,
                           std::float64_t interval_s);

// Writes the important discrete events of one scenario section (no heartbeats).
void write_event_table(std::ostream& out, std::span<const SilEvent> events);

// Appends the telemetry/event/post-fault sections of one detailed scenario section.
void write_scenario_telemetry_sections(std::ostream& out, const ScenarioRecord& record,
                                       std::float64_t telemetry_report_interval_s);

}
