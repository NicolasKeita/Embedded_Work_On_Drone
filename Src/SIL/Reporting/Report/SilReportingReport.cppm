/*
Filename: Src/SIL/Reporting/Report/SilReportingReport.cppm
Description: Report content of the SIL validation artifacts: scenario records, options and Markdown/CSV section writers.
Exports:
    enum class ReportError,
    struct ScenarioRecord,
    struct SilReportOptions,
    yes_no(),
    write_seconds(),
    write_metric(),
    write_csv_escaped(),
    write_fault_value(),
    write_markdown_report(),
    write_telemetry_csv(),
    write_truth_csv()

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReportingReport;

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
    bool                  write_text_trace = true;
    bool                  write_telemetry = true;
    std::float64_t        telemetry_report_interval_s = 1.0;
};

}

export namespace sim::sil {

[[nodiscard]] std::string_view yes_no(bool value) noexcept;
void write_seconds(std::ostream& out, std::float64_t value);
void write_metric(std::ostream& out, std::float64_t value);
void write_csv_escaped(std::ostream& out, std::string_view text);

/*
Streams the labelled numeric parameter of one fault event (efficiency, loss
probability, forced altitude or noise amplitude) with its unit and, for the
actuator efficiency, the resulting capacity loss.
*/
void write_fault_value(std::ostream& out, const SilEvent& event);

void write_markdown_report(std::ostream& out, std::span<const ScenarioRecord> records,
                           std::float64_t telemetry_report_interval_s = 1.0);

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

}

namespace sim::sil {

// Writes the periodic telemetry table of one scenario section.
void write_telemetry_table(std::ostream& out, std::span<const TelemetrySample> samples,
                           std::float64_t interval_s);

// Writes one full aligned telemetry table over the selected time window.
void write_telemetry_window(std::ostream& out, std::span<const TelemetrySample> samples,
                            std::float64_t start_after, std::float64_t stop_before, std::float64_t interval_s);

// Writes the important discrete events of one scenario section (no heartbeats).
void write_event_table(std::ostream& out, std::span<const SilEvent> events);

// True when an event belongs to the human-readable report (no heartbeat noise).
bool is_report_event(SilEventType type);

// Short category label of a report event row.
std::string_view event_category(SilEventType type);

// Streams the description of one report event (type, detail, transition, reason).
void write_event_description(std::ostream& out, const SilEvent& event);

// Appends the telemetry/event/post-fault sections of one detailed scenario section.
void write_scenario_telemetry_sections(std::ostream& out, const ScenarioRecord& record,
                                       std::float64_t telemetry_report_interval_s);

}
