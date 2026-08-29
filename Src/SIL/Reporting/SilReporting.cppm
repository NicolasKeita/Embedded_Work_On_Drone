/*
Filename: Src/SIL/Reporting/SilReporting.cppm
Description: SIL validation artifacts export : Markdown, JSON and CSV builders plus shared formatting and file-writing helpers.

Copyright (c) 2026 Nicolas K.
All rights reserved.
*/

export module SilReporting;

import std;

import SilTypes;

export namespace sim::sil {

[[nodiscard]] std::string format_seconds(double value);
[[nodiscard]] std::string format_metric(double value);
[[nodiscard]] std::string json_escape(std::string_view text);
[[nodiscard]] std::string csv_escape(std::string_view text);
[[nodiscard]] std::string_view yes_no(bool value);
void write_file(const std::filesystem::path& path, std::string_view content);

struct ScenarioRecord {
    std::string name;
    FaultScenario scenario;
    SimulationResult result;
};

struct SilReportOptions {
    std::filesystem::path docs_dir = "docs/validation";
    bool write_markdown = true;
    bool write_json = true;
    bool write_csv = true;
};

/*
Genere les artefacts de validation SIL (docs/validation/sil.md, sil.json,
sil.csv) et retourne le rapport Markdown.
*/
[[nodiscard]] std::string write_sil_report(const std::vector<ScenarioRecord>& records,
                                           const SilReportOptions& options = {});
[[nodiscard]] std::string json_payload(const std::vector<ScenarioRecord>& records);
[[nodiscard]] std::string csv_payload(const std::vector<ScenarioRecord>& records);

}
