/*
Filename: Src/SIL/Reporting/SilReporting-File.cpp
Description: SIL validation artifact orchestration and typed file writing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

import SilReportingReport;
import SilTypes;

namespace sim::sil {

/*
Streams one report section into a freshly opened file, or returns a typed
error when the file cannot be opened.
*/
std::expected<void, ReportError> write_section_file(const std::filesystem::path&    path,
                                                    ReportSectionWriter             writer,
                                                    std::span<const ScenarioRecord> records)
{
    std::ofstream file{path};

    if (!file.is_open()) {
        return std::unexpected(ReportError::FileOpen);
    }
    writer(file, records);
    return {};
}

/*
Streams the mission/fault result artifacts (Markdown, JSON, CSV) when enabled.
*/
static std::expected<void, ReportError> write_result_artifacts(std::span<const ScenarioRecord> records,
                                                               const SilReportOptions& options)
{
    if (options.write_markdown) {
        std::ofstream file{options.docs_dir / "sil.md"};

        if (!file.is_open()) {
            return std::unexpected(ReportError::FileOpen);
        }
        write_markdown_report(file, records, options.telemetry_report_interval_s);
    }
    if (options.write_json) {
        const std::expected<void, ReportError> outcome =
            write_section_file(options.docs_dir / "sil.json", &write_json_payload, records);
        if (!outcome.has_value()) {
            return outcome;
        }
    }
    if (options.write_csv) {
        return write_section_file(options.docs_dir / "sil.csv", &write_csv_payload, records);
    }
    return {};
}

/*
Streams the trace artifacts (JSONL event trace plus the raw telemetry and
ground-truth CSV streams) when enabled.
*/
static std::expected<void, ReportError> write_trace_artifacts(std::span<const ScenarioRecord> records,
                                                              const SilReportOptions& options)
{
    if (options.write_trace) {
        const std::expected<void, ReportError> outcome =
            write_section_file(options.docs_dir / "sil_trace.jsonl", &write_jsonl_trace, records);
        if (!outcome.has_value()) {
            return outcome;
        }
    }
    if (options.write_telemetry) {
        const std::expected<void, ReportError> telemetry_outcome =
            write_section_file(options.docs_dir / "sil_telemetry.csv", &write_telemetry_csv, records);
        if (!telemetry_outcome.has_value()) {
            return telemetry_outcome;
        }
        return write_section_file(options.docs_dir / "sil_truth.csv", &write_truth_csv, records);
    }
    return {};
}

/*
Generates the SIL validation artifacts (docs/validation/sil.md, sil.json,
sil.csv, sil_trace.jsonl, sil_telemetry.csv, sil_truth.csv); returns the first
typed error encountered.
*/
std::expected<void, ReportError> write_sil_report(std::span<const ScenarioRecord> records,
                                                  const SilReportOptions&         options)
{
    std::error_code ec;

    std::filesystem::create_directories(options.docs_dir, ec);
    if (ec) {
        return std::unexpected(ReportError::DirectoryCreation);
    }
    return write_result_artifacts(records, options).and_then(
        [&records, &options] { return write_trace_artifacts(records, options); });
}

}
