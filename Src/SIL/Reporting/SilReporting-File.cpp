/*
Filename: Src/SIL/Reporting/SilReporting-File.cpp
Description: SIL validation artifact orchestration and typed file writing.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

module SilReporting;

import std;

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
Generates the SIL validation artifacts (docs/validation/sil.md, sil.json,
sil.csv); returns the first typed error encountered.
*/
std::expected<void, ReportError> write_sil_report(std::span<const ScenarioRecord> records,
                                                  const SilReportOptions&         options)
{
    std::error_code ec;

    std::filesystem::create_directories(options.docs_dir, ec);
    if (ec) {
        return std::unexpected(ReportError::DirectoryCreation);
    }
    if (options.write_markdown) {
        const std::expected<void, ReportError> outcome =
            write_section_file(options.docs_dir / "sil.md", &write_markdown_report, records);
        if (!outcome.has_value()) {
            return outcome;
        }
    }
    if (options.write_json) {
        const std::expected<void, ReportError> outcome =
            write_section_file(options.docs_dir / "sil.json", &write_json_payload, records);
        if (!outcome.has_value()) {
            return outcome;
        }
    }
    if (options.write_csv) {
        const std::expected<void, ReportError> outcome =
            write_section_file(options.docs_dir / "sil.csv", &write_csv_payload, records);
        if (!outcome.has_value()) {
            return outcome;
        }
    }
    return {};
}

}
